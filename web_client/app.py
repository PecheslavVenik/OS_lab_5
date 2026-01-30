from flask import Flask, render_template, jsonify, request
import requests
import time

app = Flask(__name__)
CPP_SERVER_URL = "http://127.0.0.1:8080"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/proxy/current')
def proxy_current():
    try:
        resp = requests.get(f"{CPP_SERVER_URL}/current", timeout=2)
        if resp.status_code == 200:
            return jsonify(resp.json())
        return jsonify({"error": "Ошибка сервера C++"}), 502
    except Exception as e:
        return jsonify({"error": str(e)}), 503

@app.route('/api/proxy/stats')
def proxy_stats():
    stats_type = request.args.get('type', 'history')
    now = int(time.time())
    start = request.args.get('start', now - 86400)
    end = request.args.get('end', now)
    
    try:
        params = {'type': stats_type, 'start': start, 'end': end}
        resp = requests.get(f"{CPP_SERVER_URL}/stats", params=params, timeout=5)
        if resp.status_code == 200:
            return jsonify(resp.json())
        return jsonify([]), 502
    except:
        return jsonify([]), 503

if __name__ == '__main__':
    print("Запуск веб-клиента на http://localhost:5010")
    app.run(host='0.0.0.0', port=5010, debug=True)
