# Modbus TCP-to-RTU Gateway

A PC running a Modbus TCP client simulator connects to the Raspberry Pi 3's LAN IP address on TCP port `5020`. The Pi runs a Python gateway using PyModbus and communicates with the Arduino Uno using Modbus RTU over USB serial. The Uno runs C++ firmware using ModbusRTUSlave and exposes its digital and analog I/O.

## Architecture

```mermaid
flowchart LR
    PC["PC running a Modbus TCP simulator"] <-->|"Modbus TCP to Pi LAN IP, port 5020"| Pi["Raspberry Pi 3 gateway and cache"]
    Pi <-->|"RTU over USB serial, 9600 8N1"| Uno["Arduino Uno, unit ID 1"]
```

| Component | Role |
| --- | --- |
| PC | Runs the Modbus TCP client simulator and sends requests to the Pi's IP address. |
| [Pi gateway](ModbusRTUSlave/PyModbusTCPtoRTU.py) | Serves Modbus TCP requests from a local datastore and acts as the Modbus RTU client on the serial link. |
| [Uno firmware](ModbusRTUSlave/src/main.cpp) | Acts as the RTU server. Samples inputs and drives outputs from the coil states. |

The Pi polls the Uno and copies successful RTU reads into the TCP datastore. TCP writes update that datastore first, then a separate loop sends changed coil and holding-register values over RTU. A successful TCP write response confirms the cache update. Check the gateway log and the physical output for evidence that a coil write reached the Uno.

## Uno I/O map

Addresses below are zero-based offsets within each Modbus data table.

| Data table | Offsets | Uno pins and behavior |
| --- | --- | --- |
| Coils | 0–1 | D6–D7 digital outputs |
| Discrete inputs | 0–3 | D2–D5 with internal pull-ups. Open reads 1, grounded reads 0. |
| Input registers | 0–1 | A0–A1 raw ADC readings |
| Holding registers | 0–1 | A0–A1 raw ADC readings mirrored by the firmware. Each loop overwrites register writes. |

The gateway's write loop scans offsets 0–9 for coils and holding registers, while the Uno implements only offsets 0–1 in those tables. Use the Uno map above when testing. After a successful RTU write, the gateway marks that cache entry as pending and never clears the flag, so TCP reads of that entry can continue to show the cached value. The poll and write loops share one serial client without a lock, so their RTU requests can overlap.

## Setup

1. Install [ModbusRTUSlave](https://github.com/CMB27/ModbusRTUSlave) 3.1.2 and its dependencies in the Arduino IDE. Copy `ModbusRTUSlave/src/main.cpp` into a new sketch as its `.ino` contents, then select **Arduino Uno** and upload.
2. Connect the Uno to the Pi over USB. In [the gateway script](ModbusRTUSlave/PyModbusTCPtoRTU.py), confirm `/dev/ttyACM0` matches the Uno's serial device and that your Pi user can open it. Set the gateway's TCP bind address (default `10.0.0.93`) to the Pi's LAN IP. Enter that same address in the PC simulator. The TCP port defaults to `5020`, and the Uno's Modbus unit ID is `1`.
3. From the repository root on the Pi, create a virtual environment and start the gateway. The setup uses library versions that expose the APIs used by the code. The repository does not record the original test environment's versions.

   ```sh
   python3 -m venv .venv
   . .venv/bin/activate
   python -m pip install 'pymodbus==3.11.3' pyserial
   python ModbusRTUSlave/PyModbusTCPtoRTU.py
   ```

## Bench test

1. On the PC, open the Modbus TCP client simulator and connect to the Pi's LAN IP on port `5020`. Set the request unit ID to `1` for the Uno. Use zero-based addressing or the simulator's equivalent display addresses.
2. Read four discrete inputs from offset 0 (FC02). Momentarily ground D2 and confirm bit 0 changes from 1 to 0, then returns to 1 when released.
3. Read two input registers from offset 0 (FC04). Vary a potentiometer wired between the Uno's 5 V and GND with its wiper on A0. Confirm register 0 follows the change.
4. Write coil 0 on and off (FC05). Verify D6 changes state with a meter or an LED with a series resistor. Check `modbus_gateway.log` for the matching `Wrote coil 0` entries.

Simulator screenshots, wiring photos, packet captures, and measured latency or throughput are absent from the repository.

## Resume bullet

- Built a Modbus TCP-to-RTU prototype with a Python/PyModbus gateway on a Raspberry Pi 3 and C++ firmware on an Arduino Uno. Mapped analog and digital I/O to Modbus data tables and exercised the gateway with a Modbus TCP client simulator.
