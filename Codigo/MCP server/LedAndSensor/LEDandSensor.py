import logging
import threading
import time
from typing import Optional

import paho.mqtt.client as mqtt
from fastmcp import FastMCP


# =========================
# Logging
# =========================
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(threadName)s | %(message)s"
)
logger = logging.getLogger("iot_mcp_server")


# =========================
# MQTT config
# =========================
BROKER = "broker.hivemq.com"
PORT = 1883

BASE = "IoT/ZubietaVargas/"

DIST_TOPIC = BASE + "sensor/distance"
GET_DIST_TOPIC = BASE + "sensor/get"

# =========================
# Shared state
# =========================
last_distance: Optional[str] = None
state_lock = threading.Lock()


# =========================
# MQTT callbacks
# =========================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logger.info("Connected to MQTT broker")
        client.subscribe(DIST_TOPIC)
        logger.info("Subscribed to %s", DIST_TOPIC)
    else:
        logger.error("MQTT connection failed: %s", rc)


def on_message(client, userdata, msg):
    global last_distance

    payload = msg.payload.decode()

    if msg.topic == DIST_TOPIC:
        with state_lock:
            last_distance = payload

        logger.info("Distance received: %s cm", payload)


# =========================
# MQTT setup
# =========================
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message


def start_mqtt():
    while True:
        try:
            logger.info("Connecting to MQTT...")
            mqtt_client.connect(BROKER, PORT, 60)
            mqtt_client.loop_forever()
        except Exception as e:
            logger.exception("MQTT error: %s", e)
            time.sleep(5)


# =========================
# MCP Server
# =========================
mcp = FastMCP("IoT LED + Sensor Server")


# -------------------------
# LED CONTROL TOOL
# -------------------------
@mcp.tool()
def set_led(color: str, action: str) -> str:
    """
    Control an LED.

    color: green, yellow, red, blue
    action: on, off, blink:X  (X = blinks/sec)
    """

    color = color.lower()
    action = action.upper()

    valid_colors = ["green", "yellow", "red", "blue"]

    if color not in valid_colors:
        return f"Invalid color. Use {valid_colors}"

    topic = f"{BASE}led/{color}/set"

    # validar acción
    if action not in ["ON", "OFF"] and not action.startswith("BLINK"):
        return "Invalid action. Use ON, OFF or BLINK:X"

    result = mqtt_client.publish(topic, action)

    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        logger.info("Sent %s to %s", action, topic)
        return f"{color} LED -> {action}"
    else:
        return "MQTT publish failed"


# -------------------------
# GET DISTANCE TOOL
# -------------------------
@mcp.tool()
def get_distance() -> str:
    """
    Request and return the latest ultrasonic distance in cm.
    """

    # pedir medición
    mqtt_client.publish(GET_DIST_TOPIC, "1")

    time.sleep(0.5)  # pequeña espera

    with state_lock:
        dist = last_distance

    if dist is None:
        return "No distance data yet"

    return f"{dist} cm"


# =========================
# MAIN
# =========================
def main():
    mqtt_thread = threading.Thread(
        target=start_mqtt,
        name="MQTT-Thread",
        daemon=True
    )
    mqtt_thread.start()

    logger.info("Starting MCP server...")
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()