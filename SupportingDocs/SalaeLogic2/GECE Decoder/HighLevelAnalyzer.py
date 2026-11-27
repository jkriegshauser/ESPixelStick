from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting, NumberSetting

class GeceHla(HighLevelAnalyzer):
    bit_period_us = NumberSetting(label='Bit Period (µs)', min_value=15.0, max_value=60.0)
    idle_polarity = ChoicesSetting(label='Idle Line State', choices=('Low (Standard)', 'High (Inverted)'))

    result_types = {
        'gece_packet': {
            'format': 'Addr:{{data.addr}} {{data.bcast}} | Bri:{{data.bri}} | RGB({{data.r}},{{data.g}},{{data.b}}) [{{data.hex}}]'
        },
        'gece_error': {
            'format': 'GECE Error: {{data.reason}}'
        }
    }

    def __init__(self):
        self.t_bit = (float(self.bit_period_us) if self.bit_period_us else 30.0) * 1e-6
        self.t_half = self.t_bit * 0.5
        self.t_reset = self.t_bit * 1.4  # >42 µs idle gap resets packet

        self.inverted = (self.idle_polarity == 'High (Inverted)')
        self.dt_sample = 1e-6  # 1 µs per sample at 1,000,000 bps

        self.last_level = 0
        self.last_rise_time = None
        self.high_duration = 0.0

        self.in_packet = False
        self.packet_start_time = None
        self.bits = []

    def _reset_packet(self):
        self.in_packet = False
        self.packet_start_time = None
        self.bits = []

    def _decode_bits(self, end_time):
        val = 0
        for bit in self.bits:
            val = (val << 1) | bit

        addr = (val >> 20) & 0x3F
        bri = (val >> 12) & 0xFF
        blue = (val >> 8) & 0x0F
        green = (val >> 4) & 0x0F
        red = val & 0x0F

        return AnalyzerFrame('gece_packet', self.packet_start_time, end_time, {
            'addr': addr,
            'bcast': '(BCAST)' if addr == 63 else '',
            'bri': bri,
            'r': red,
            'g': green,
            'b': blue,
            'hex': f'0x{val:07X}'
        })

    def _process_edge(self, level, edge_time):
        out_frame = None
        if level == 0:
            # Falling edge: record how long the pulse was high
            if self.last_rise_time is not None:
                self.high_duration = float(edge_time - self.last_rise_time)
        else:
            # Rising edge
            if self.last_rise_time is not None:
                period = float(edge_time - self.last_rise_time)
                if period > self.t_reset:
                    if self.in_packet and 0 < len(self.bits) < 26:
                        out_frame = AnalyzerFrame('gece_error', self.packet_start_time, edge_time, {
                            'reason': f'Incomplete ({len(self.bits)}/26 bits)'
                        })
                    self.in_packet = True
                    self.packet_start_time = edge_time
                    self.bits = []
                elif not self.in_packet:
                    self.in_packet = True
                    self.packet_start_time = edge_time
                    self.bits = []
                else:
                    # Skip the start bit pulse; decode the next 26 data pulses
                    if self.last_rise_time != self.packet_start_time:
                        # GECE: short high (~10 µs) = 1, long high (~20 µs) = 0
                        bit = 1 if self.high_duration < self.t_half else 0
                        self.bits.append(bit)
                        if len(self.bits) == 26:
                            out_frame = self._decode_bits(edge_time)
                            self._reset_packet()
            else:
                self.in_packet = True
                self.packet_start_time = edge_time
                self.bits = []

            self.last_rise_time = edge_time
        return out_frame

    def decode(self, frame: AnalyzerFrame):
        if frame.type != 'data':
            return None

        raw_data = frame.data['data']
        byte_val = raw_data[0] if isinstance(raw_data, (bytes, bytearray)) else int(raw_data)
        if self.inverted:
            byte_val ^= 0xFF

        # Reconstruct 10 samples of the UART frame: [Start=0, b0..b7, Stop=1]
        samples = [0] + [(byte_val >> i) & 1 for i in range(8)] + [1]

        results = []
        # If line was low before this frame's falling start edge, synthesize the missing rising edge
        if self.last_level == 0:
            # Estimate rising edge occurred ~10-20 us before falling edge, or at previous end
            res = self._process_edge(1, frame.start_time - self.dt_sample)
            if res:
                results.append(res)
            self.last_level = 1

        for idx, s in enumerate(samples):
            if s != self.last_level:
                t = frame.start_time + (idx * self.dt_sample)
                res = self._process_edge(s, t)
                if res:
                    results.append(res)
                self.last_level = s

        return results if results else None
