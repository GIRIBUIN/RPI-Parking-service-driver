# $BrokerHost = "localhost"
$BrokerHost = "192.168.4.1"
$MosquittoPub = "C:\Program Files\Mosquitto\mosquitto_pub.exe"

function Publish-Mqtt {
    param (
        [string]$Topic,
        [string]$Message
    )

    if (-not (Test-Path $MosquittoPub)) {
        Write-Error "mosquitto_pub.exe not found: $MosquittoPub"
        exit 1
    }

    Write-Host "PUB $Topic => $Message"
    & $MosquittoPub -h $BrokerHost -t $Topic -m $Message
    Start-Sleep -Milliseconds 700
}

Write-Host "=== Parking Monitoring Simulation Start ==="

# 1. Initial state
Publish-Mqtt "parking/rpi1/slot" '{"slot1":"EMPTY","slot2":"EMPTY"}'
Publish-Mqtt "parking/rpi1/distance" '{"slot1":50,"slot2":50,"unit":"cm"}'
Publish-Mqtt "parking/rpi1/lot" "AVAILABLE"
Publish-Mqtt "parking/rpi2/gate" "CLOSED"

# 2. Vehicle approaches entrance and gate opens
Publish-Mqtt "parking/rpi2/event" '{"event":"ENTRY_OPEN","reason":"vehicle_detected"}'
Publish-Mqtt "parking/rpi2/gate" "OPEN"

# 3. Gate closes after entry timer
Publish-Mqtt "parking/rpi2/gate" "CLOSED"

# 4. Vehicle parks in Slot 1
Publish-Mqtt "parking/rpi1/slot" '{"slot1":"OCCUPIED","slot2":"EMPTY"}'
Publish-Mqtt "parking/rpi1/distance" '{"slot1":12,"slot2":50,"unit":"cm"}'
Publish-Mqtt "parking/rpi1/lot" "AVAILABLE"

# 5. Another vehicle enters
Publish-Mqtt "parking/rpi2/event" '{"event":"ENTRY_OPEN","reason":"vehicle_detected"}'
Publish-Mqtt "parking/rpi2/gate" "OPEN"
Publish-Mqtt "parking/rpi2/gate" "CLOSED"

# 6. Vehicle parks in Slot 2, parking lot becomes full
Publish-Mqtt "parking/rpi1/slot" '{"slot1":"OCCUPIED","slot2":"OCCUPIED"}'
Publish-Mqtt "parking/rpi1/distance" '{"slot1":12,"slot2":10,"unit":"cm"}'
Publish-Mqtt "parking/rpi1/lot" "FULL"

# 7. Vehicle approaches while lot is full
# Realistic scenario:
# The gate opens, but Dashboard/LED indicates FULL and the vehicle turns around inside.
Publish-Mqtt "parking/rpi2/event" '{"event":"ENTRY_OPEN","reason":"vehicle_detected_full_lot"}'
Publish-Mqtt "parking/rpi2/gate" "OPEN"
Publish-Mqtt "parking/rpi2/gate" "CLOSED"

# 8. Exit request
Publish-Mqtt "parking/rpi2/event" '{"event":"EXIT_OPEN","reason":"switch_pressed"}'
Publish-Mqtt "parking/rpi2/gate" "OPEN"

# 9. Slot 1 becomes empty after vehicle exits
Publish-Mqtt "parking/rpi1/slot" '{"slot1":"EMPTY","slot2":"OCCUPIED"}'
Publish-Mqtt "parking/rpi1/distance" '{"slot1":50,"slot2":10,"unit":"cm"}'
Publish-Mqtt "parking/rpi1/lot" "AVAILABLE"

# 10. Gate closes after exit timer
Publish-Mqtt "parking/rpi2/gate" "CLOSED"

Write-Host "=== Parking Monitoring Simulation End ==="
