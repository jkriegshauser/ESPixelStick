# GE Color Effects (GECE) Analyzer
Decodes 26-bit GE Color Effects LED pixel frames (Address, Brightness, Red, Green, Blue) in Saleae Logic 2.

## Setup
1. Add the built-in **Simple Parallel** analyzer to your capture.
2. Assign **D0** to your GECE data channel and leave **Clock** set to `None`.
3. Add **GECE Decoder** from the Analyzers `+` menu and select **Simple Parallel** as the Input Analyzer.
