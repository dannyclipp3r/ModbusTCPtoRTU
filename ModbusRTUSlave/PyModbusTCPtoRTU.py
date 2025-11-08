# File:   PyModbusTCPtoRTU.py
# Author: Daniel Clipper
# Date:   10/27/2025


import logging
import asyncio
from pymodbus.server import StartAsyncTcpServer
from pymodbus.client import ModbusSerialClient
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusDeviceContext, ModbusServerContext


logging.basicConfig(
    filename='modbus_gateway.log',
    filemode='a',                  
    format='%(asctime)s [%(levelname)s] %(message)s',
    level=logging.INFO            
)


def client_connect(client):
    if not client.connect():
        logging.error("Serial Client Cannot Connect...")
        exit(1)
    logging.info("Connected to RTU device")

pending_coils = [False]*10
pending_hr = [False]*10

async def serial_write_loop(client, context):
    prev_coils = [None]*10
    prev_hr = [None]*10
    global pending_coils, pending_hr

    while True:
        # Coils
        coils = context[1].getValues(1, 0, 10)
        for i, val in enumerate(coils):
            if prev_coils[i] != val:
                wr = await asyncio.to_thread(client.write_coils, i, [val])
                if not wr.isError():
                    logging.info(f"Wrote coil {i} = {val}")
                    prev_coils[i] = val
                    pending_coils[i] = True  # mark as pending

        # Holding Registers
        hrs = context[1].getValues(3, 0, 10)
        for i, val in enumerate(hrs):
            if prev_hr[i] != val:
                wr = await asyncio.to_thread(client.write_registers, i, [val])
                if not wr.isError():
                    logging.info(f"Wrote HR {i} = {val}")
                    prev_hr[i] = val
                    pending_hr[i] = True  # mark as pending

        await asyncio.sleep(0.2)

async def serial_poll_loop(client, context):
   
    global pending_coils, pending_hr

    while True:
        # --- Coils
        coil_rr = await asyncio.to_thread(client.read_coils, address=0, count=2, device_id=1)
        if not coil_rr.isError():
            for i, val in enumerate(coil_rr.bits[:10]):
                if not pending_coils[i]:
                    context[1].setValues(1, i, [val])

        # --- Discrete Inputs
        di_rr = await asyncio.to_thread(client.read_discrete_inputs,address=0, count=4, device_id=1)
        if not di_rr.isError():
            context[1].setValues(2, 0, di_rr.bits[:4])

        # --- Holding Registers
        hr_rr = await asyncio.to_thread(client.read_holding_registers, address=0, count=2, device_id=1)
        if not hr_rr.isError():
            for i, val in enumerate(hr_rr.registers[:10]):
                if not pending_hr[i]:
                    context[1].setValues(3, i, [val])

        # --- Input Registers
        ir_rr = await asyncio.to_thread(client.read_input_registers, address=0, count=2, device_id=1)
        if not ir_rr.isError():
            context[1].setValues(4, 0, ir_rr.registers[:10])

        await asyncio.sleep(0.5)

    
async def main():
    client = ModbusSerialClient(
        port="/dev/ttyACM0",
        baudrate=9600,
        parity='N',
        stopbits=1,
        bytesize=8,
        timeout=2
    )

    store = ModbusDeviceContext(
        di=ModbusSequentialDataBlock(0, [0]*64),
        co=ModbusSequentialDataBlock(0, [0]*64),
        hr=ModbusSequentialDataBlock(0, [0]*64),
        ir=ModbusSequentialDataBlock(0, [0]*64),
    )
    context = ModbusServerContext(devices={1:store}, single=False)

    client_connect(client)

    await asyncio.gather(
        StartAsyncTcpServer(context, address=("10.0.0.93", 5020)),
        serial_write_loop(client, context),
        serial_poll_loop(client, context)
    )


if __name__ == "__main__":
    asyncio.run(main())

