import os
import json
import paho.mqtt.client as mqtt
import mysql.connector
import logging # <-- Import the logging module

# --- Logging Setup ---
logging.basicConfig(
    level=logging.INFO, # Display INFO, WARNING, ERROR, CRITICAL messages
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

# --- Configuration (Defaults from docker-compose.yml) ---
MQTT_HOST = os.getenv("MQTT_HOST", "SITH-MQTT-Broker")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "roverCommsLog/#") # Subscribing to all topics published by the publisher

DB_HOST = os.getenv("DB_HOST", "SITH-MySQL")
DB_NAME = os.getenv("DB_NAME", "SITH")
DB_USER = os.getenv("DB_USER", "SITH")
DB_PASS = os.getenv("DB_PASS", "SITH")

# --- NEW: Define the table where incoming MQTT data will be logged ---
TARGET_DB_TABLE = "mqtt_data_log"
TOPIC_PREFIX = "roverCommsLog/"

# Global DB connection pool
DB_POOL = None 

# --- Database Functions ---
def setup_db_pool():
    """Initializes the MySQL connection pool."""
    global DB_POOL
    logger.info(f"Setting up DB connection pool for {DB_USER}@{DB_HOST}/{DB_NAME}...")
    try:
        DB_POOL = mysql.connector.pooling.MySQLConnectionPool(
            pool_name="mqtt_pool",
            pool_size=5,
            host=DB_HOST,
            database=DB_NAME,
            user=DB_USER,
            password=DB_PASS
        )
        logger.info("Database connection pool established.")
    except Exception as e:
        logger.error(f"Error setting up DB connection pool: {e}", exc_info=True)
        # Exit if DB connection fails, as the service cannot function
        exit(1)

def insert_data(device_id, value, full_topic):
    """Inserts a new MQTT reading into the database."""
    conn = None
    try:
        conn = DB_POOL.get_connection()
        cursor = conn.cursor()
        
        # SQL INSERT statement using parameterized query for safety
        sql = f"""
        INSERT INTO {TARGET_DB_TABLE} 
        (device_id, value_payload, mqtt_topic, received_at)
        VALUES (%s, %s, %s, NOW());
        """
        cursor.execute(sql, (device_id, value, full_topic))
        conn.commit()
        logger.debug(f"Inserted: ID={device_id}, Value={value} from topic {full_topic}")
        
    except Exception as e:
        if conn:
            conn.rollback()
        # Log specific error if the table or columns are missing
        error_message = str(e)
        if f"'{TARGET_DB_TABLE}' doesn't exist" in error_message or "Unknown column" in error_message:
             logger.error(f"Database error: Table '{TARGET_DB_TABLE}' or its required columns (device_id, value_payload, mqtt_topic) are missing.")
        else:
             logger.error(f"Database insertion error: {e}")
        
    finally:
        if conn and conn.is_connected():
            cursor.close()
            conn.close()

# --- MQTT Callbacks ---
def on_connect(client, userdata, flags, rc, properties):
    """Callback triggered upon connection to MQTT broker."""
    if rc == 0:
        logger.info(f"Connected to MQTT Broker successfully.")
        client.subscribe(MQTT_TOPIC)
        logger.info(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        logger.error(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    """Callback triggered when a message is received."""
    full_topic = msg.topic
    
    # Check if the topic starts with the expected prefix
    if not full_topic.startswith(TOPIC_PREFIX):
        logger.debug(f"Skipping non-matching topic: {full_topic}")
        return
    
    try:
        # The 'publisher.py' script uses the topic suffix as the device ID (roverID)
        # Example: Topic is "roverCommsLog/rover_A". Device ID is "rover_A".
        device_id = full_topic[len(TOPIC_PREFIX):]
        
        # The payload is the raw value (e.g., '24.5' or 'COMMAND_SENT')
        value_payload = msg.payload.decode('utf-8')
        
        if not device_id or not value_payload:
             logger.warning(f"Skipping message with empty device ID or payload on topic {full_topic}")
             return

        # Insert the data into the log table
        insert_data(device_id, value_payload, full_topic)

    except Exception as e:
        logger.error(f"Error processing message on topic {full_topic}: {e}", exc_info=True)

# --- Main Execution ---
if __name__ == "__main__":
    logger.info("MQTT Subscriber service started.")
    
    # 1. Initialize the Database Connection Pool
    setup_db_pool()
    
    # 2. Setup the MQTT Client
    mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    try:
        mqtt_client.connect(MQTT_HOST, MQTT_PORT, 60)
        # Blocks and handles network loop (reconnections, message handling)
        logger.info("Awaiting MQTT messages...")
        mqtt_client.loop_forever()
    except Exception as e:
        logger.error(f"MQTT connection failed: {e}")