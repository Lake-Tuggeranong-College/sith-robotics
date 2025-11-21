# Example Python-based Dockerfile
FROM python:3.10-slim

WORKDIR /app

# Install necessary libraries (paho-mqtt for MQTT, and a MySQL connector)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY subscriber_main.py .

CMD ["python", "subscriber_main.py"]