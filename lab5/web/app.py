import os
import sqlite3
import re
import json
from datetime import datetime, timedelta
from flask import Flask, render_template, jsonify, request

app = Flask(__name__)

DB_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'build', 'temperature.db')


def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/api/current')
def api_current():
    conn = get_db_connection()
    cur = conn.execute(
        "SELECT temperature, timestamp FROM main ORDER BY timestamp DESC LIMIT 1"
    )
    row = cur.fetchone()
    conn.close()
    if row:
        return jsonify({
            'temperature': round(row['temperature'], 2),
            'timestamp': row['timestamp']
        })
    return jsonify({'error': 'no data'})


@app.route('/api/main')
def api_measurements():
    conn = get_db_connection()
    
    start = request.args.get('start')
    end = request.args.get('end')

    if start and end:
        if not re.match(r'^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$', start) or \
           not re.match(r'^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$', end):
            return jsonify({'error': 'Invalid date format. Use YYYY-MM-DD HH:MM:SS'}), 400
        
        cur = conn.execute(
            "SELECT temperature, timestamp FROM main "
            "WHERE timestamp BETWEEN ? AND ? "
            "ORDER BY timestamp",
            (start, end)
        )
        #http://localhost:5000/api/main?start=2026-01-09%2003:57:15&end=2026-01-09%2003:57:18
    else:
        cur = conn.execute(
            "SELECT temperature, timestamp FROM main "
            "ORDER BY timestamp DESC LIMIT 20"
        )
        rows = cur.fetchall()
        data = [{'time': row['timestamp'], 'temp': row['temperature']} for row in reversed(rows)]
        conn.close()
        return jsonify(data)

    data = [{'time': row['timestamp'], 'temp': row['temperature']} for row in cur.fetchall()]
    conn.close()
    return jsonify(data)


@app.route('/api/hourly')
def api_hourly():
    conn = get_db_connection()
    cur = conn.execute(
        "SELECT hour_start, avg_temp FROM hourly_avg WHERE hour_start >= datetime('now', '-30 days') ORDER BY hour_start DESC"
    )
    data = [
        {'hour': row['hour_start'], 'avg': round(row['avg_temp'], 2)}
        for row in cur.fetchall()
    ]
    conn.close()
    return jsonify(data)


@app.route('/api/daily')
def api_daily():
    conn = get_db_connection()
    cur = conn.execute(
        "SELECT day_start, avg_temp FROM daily_avg ORDER BY day_start DESC"
    )
    data = [
        {'date': row['day_start'], 'avg': round(row['avg_temp'], 2)}
        for row in cur.fetchall()
    ]
    conn.close()
    return jsonify(data)


if __name__ == '__main__':

    if not os.path.exists(DB_PATH):
        print(f"Warning: database not found at {DB_PATH}")
    else:
        print(f"Database found at {DB_PATH}")
    app.run(debug=True, host='0.0.0.0', port=5000)