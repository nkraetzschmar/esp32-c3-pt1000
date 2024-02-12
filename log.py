#!/usr/bin/env python3

import asyncio
import datetime
import time
import sys

from bleak import BleakClient, BleakScanner, BleakError

SERVICE_UUID = "621ABF0E-E642-40D8-B766-8D910900B1C5"
CHARACTERISTIC_UUID = "E33E3DC8-BCB7-476A-8D26-1B6D968F615C"

connect_timeout = 30

async def scan():
	def device_filter(device, advertisement_data):
		return SERVICE_UUID.lower() in [ uuid.lower() for uuid in advertisement_data.service_uuids ]

	global device
	device = await BleakScanner.find_device_by_filter(device_filter, timeout=connect_timeout)
	if device:
		print(f"Found device: {device.name} [{device.address}]", file=sys.stderr)
	else:
		raise RuntimeError("No device found advertising the specified service UUID")
	
async def connect():
	def on_disconnect(client):
		print(f"Disconnected from {client.address}", file=sys.stderr)
		asyncio.get_running_loop().create_task(connect())

	client = BleakClient(device.address, disconnected_callback=on_disconnect)

	t = time.time()
	while time.time() - t < connect_timeout:
		print("Trying to connect...", file=sys.stderr)
		try:
			await client.connect()
			break
		except (BleakError, TimeoutError):
			pass
		await asyncio.sleep(0.1)

	if not client.is_connected:
		raise RuntimeError("Failed to connect to the device, connection timeout exceeded")

	print(f"Connected to {device.name}", file=sys.stderr)

	def on_notify(sender, data):
		real_time = datetime.datetime.now(tz=datetime.timezone.utc).replace(microsecond=0).isoformat()
		time, temp = map(float, data.split())
		print(f"{real_time}, {time:.2f}, {temp:.2f}", flush=True)

	await client.start_notify(CHARACTERISTIC_UUID, on_notify)

async def init():
	await scan()
	await connect()

if __name__ == "__main__":
	print("real time, system time, temperature °C")
	loop = asyncio.new_event_loop()
	loop.create_task(init())
	try:
		loop.run_forever()
	except KeyboardInterrupt:
		pass
