$MosquittoPub = "C:\Program Files\Mosquitto\mosquitto_pub.exe"
$HostName = "localhost"
$DelayMs = 200

function Publish-Message {
    param (
        [string]$Topic,
        [string]$Message
    )

    & $MosquittoPub -h $HostName -t $Topic -m $Message
}

function Publish-Frame {
    param (
        [string]$ParkingState,
        [string]$RiskState,
        [string]$GateState,
        [double]$Front,
        [double]$Rear,
        [double]$Side
    )

    $distance = @{
        front = $Front
        rear = $Rear
        side = $Side
    } | ConvertTo-Json -Compress

    Publish-Message "parking/rpi1/status" $ParkingState
    Publish-Message "parking/rpi1/risk" $RiskState
    Publish-Message "parking/rpi1/distance" $distance
    Publish-Message "parking/rpi2/gate" $GateState
    Start-Sleep -Milliseconds $DelayMs
}

function Publish-Event {
    param ([string]$EventName)

    Publish-Message "parking/rpi1/event" $EventName
}

while ($true) {
    Publish-Frame "EMPTY" "SAFE" "OPEN" 50 80 26
    Publish-Frame "EMPTY" "SAFE" "OPEN" 50 80 26

    Publish-Event "vehicle_waiting_at_barrier"
    foreach ($front in 50, 44, 38, 32, 26, 20) {
        Publish-Frame "APPROACHING" "SAFE" "OPEN" $front 80 26
    }

    foreach ($front in 16, 12, 8, 4, 0) {
        Publish-Frame "APPROACHING" "SAFE" "OPEN" $front 75 26
    }

    Publish-Event "rear_tracking_started"
    foreach ($frame in @(
        @{ front = 0; rear = 50; side = 22; risk = "SAFE" },
        @{ front = 0; rear = 46; side = 22; risk = "SAFE" },
        @{ front = 0; rear = 42; side = 21; risk = "SAFE" },
        @{ front = 0; rear = 36; side = 21; risk = "SAFE" },
        @{ front = 0; rear = 30; side = 20; risk = "SAFE" },
        @{ front = 0; rear = 24; side = 20; risk = "SAFE" }
    )) {
        Publish-Frame "PARKING" $frame.risk "OPEN" $frame.front $frame.rear $frame.side
    }

    Publish-Event "rear_warning"
    foreach ($frame in @(
        @{ front = 0; rear = 18; side = 20; risk = "WARNING" },
        @{ front = 0; rear = 14; side = 20; risk = "WARNING" },
        @{ front = 0; rear = 11; side = 19; risk = "WARNING" }
    )) {
        Publish-Frame "PARKING" $frame.risk "OPEN" $frame.front $frame.rear $frame.side
    }

    Publish-Event "rear_danger"
    Publish-Frame "PARKING" "DANGER" "OPEN" 0 9 19
    Publish-Frame "PARKING" "DANGER" "OPEN" 0 7 19

    Publish-Event "rear_collision"
    Publish-Frame "PARKING" "DANGER" "OPEN" 0 4 19
    Publish-Frame "PARKING" "DANGER" "OPEN" 0 4 19

    Publish-Event "move_forward_to_recover"
    foreach ($frame in @(
        @{ front = 0; rear = 8; side = 19; risk = "DANGER" },
        @{ front = 0; rear = 12; side = 19; risk = "WARNING" },
        @{ front = 0; rear = 16; side = 19; risk = "WARNING" },
        @{ front = 0; rear = 19; side = 19; risk = "SAFE" }
    )) {
        Publish-Frame "PARKING" $frame.risk "OPEN" $frame.front $frame.rear $frame.side
    }

    Publish-Event "parking_completed"
    Publish-Frame "PARKED" "SAFE" "OPEN" 0 19 19
    Publish-Frame "PARKED" "SAFE" "OPEN" 0 19 19

    Publish-Message "parking/rpi2/event" "gate_closed"
    Publish-Frame "PARKED" "SAFE" "CLOSED" 0 19 19
    Start-Sleep -Milliseconds 1000
}
