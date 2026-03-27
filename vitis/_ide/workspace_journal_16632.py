# 2026-03-26T20:26:44.402990700
import vitis

client = vitis.create_client()
client.set_workspace(path="vitis")

platform = client.get_component(name="platform")
status = platform.build()

vitis.dispose()

