#!/bin/bash
# Grid Martin Paper Trading Auto-restart Script
# This script monitors the strategy and restarts it automatically if it crashes

LOG_FILE="/tmp/grid_paper_trading.log"
STRATEGY_BIN="/home/ubuntu/project/bitquant/bitquant_cpp/build/demo_grid_martin_paper"
STRATEGY_ARGS="--grid-count 10 --grid-spacing 0.01 --amount-per-grid 100"

echo "========================================"
echo "Grid Martin Paper Trading - Auto-restart"
echo "========================================"
echo "Log file: $LOG_FILE"
echo "Started at: $(date)"
echo ""

# Create a marker file to track restart count
RESTART_COUNT_FILE="/tmp/grid_martin_restart_count"
if [ ! -f "$RESTART_COUNT_FILE" ]; then
    echo "0" > "$RESTART_COUNT_FILE"
fi

# Function to handle graceful shutdown
shutdown_handler() {
    echo ""
    echo "Received shutdown signal, stopping..."
    if [ ! -z "$PID" ]; then
        kill $PID 2>/dev/null
        wait $PID 2>/dev/null
    fi
    echo "Strategy stopped."
    exit 0
}

trap shutdown_handler SIGINT SIGTERM

# Main loop
while true; do
    # Clear log on first start
    if [ ! -f "$RESTART_COUNT_FILE" ] || [ $(cat "$RESTART_COUNT_FILE") -eq 0 ]; then
        rm -f "$LOG_FILE"
        echo "0" > "$RESTART_COUNT_FILE"
    fi

    echo "Starting strategy..."
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Strategy starting..." >> "$LOG_FILE"

    # Start the strategy
    $STRATEGY_BIN $STRATEGY_ARGS >> "$LOG_FILE" 2>&1 &
    PID=$!

    # Wait for process to exit
    wait $PID
    EXIT_CODE=$?

    # Increment restart counter
    COUNT=$(cat "$RESTART_COUNT_FILE")
    COUNT=$((COUNT + 1))
    echo "$COUNT" > "$RESTART_COUNT_FILE"

    echo ""
    echo "========================================"
    echo "Strategy exited with code: $EXIT_CODE"
    echo "Restart count: $COUNT"
    echo "Restarting in 5 seconds..."
    echo "========================================"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Strategy exited (code $EXIT_CODE), restart #$COUNT" >> "$LOG_FILE"

    sleep 5
done
