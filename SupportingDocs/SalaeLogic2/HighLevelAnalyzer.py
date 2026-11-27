from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame


class Hla(HighLevelAnalyzer):

    result_types = {
        "gece": {
            "format":
                "Addr={{data.address}} "
                "Bright={{data.brightness}} "
                "RGB=({{data.red}},{{data.green}},{{data.blue}}) "
                "HEX=#{{data.hexcolor}}"
        },
        "broadcast": {
            "format":
                "Broadcast Brightness={{data.brightness}}"
        },
    }

    def __init__(self):
        self.bits = []
        self.start_time = None

    def decode(self, frame: AnalyzerFrame):

        bit_text = frame.data.get("data", "")

        if bit_text not in ["0", "1"]:
            return None

        if self.start_time is None:
            self.start_time = frame.start_time

        self.bits.append(int(bit_text))

        if len(self.bits) < 26:
            return None

        value = 0

        for b in self.bits:
            value = (value << 1) | b

        self.bits = []

        end_time = frame.end_time

        address = (value >> 20) & 0x3F
        brightness = (value >> 12) & 0xFF
        blue = (value >> 8) & 0x0F
        green = (value >> 4) & 0x0F
        red = value & 0x0F

        # Expand 4-bit color to 8-bit.
        red8 = (red << 4) | red
        green8 = (green << 4) | green
        blue8 = (blue << 4) | blue

        hexcolor = (
            f"{red8:02X}"
            f"{green8:02X}"
            f"{blue8:02X}"
        )

        if address == 63:
            return AnalyzerFrame(
                "broadcast",
                self.start_time,
                end_time,
                {
                    "brightness": brightness
                }
            )

        return AnalyzerFrame(
            "gece",
            self.start_time,
            end_time,
            {
                "address": address,
                "brightness": brightness,
                "red": red,
                "green": green,
                "blue": blue,
                "hexcolor": hexcolor
            }
        )
