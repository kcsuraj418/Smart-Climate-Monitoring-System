from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route("/", methods=["POST"])
def handle_temperature():
    # Retrieve JSON payload
    data = request.get_json()
    if not data:
        return jsonify({"error": "No JSON payload provided"}), 400

    # Extract temperature and humidity
    temperature = data.get("temperature")
    humidity = data.get("humidity")

    if temperature is not None and humidity is not None:
        print(f"Received temperature: {temperature} °C, humidity: {humidity} %")
        return jsonify({"message": "Data received successfully"}), 200
    else:
        return jsonify({"error": "Missing temperature or humidity"}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)