function valueOrDash(value, unit) {
    if (value === null || value === undefined) {
        return "-";
    }
    return unit ? `${value} ${unit}` : value;
}

function setText(id, value) {
    document.getElementById(id).textContent = value;
}

function vehicleLeftFromFront(frontDistance) {
    if (frontDistance === null || frontDistance === undefined) {
        return null;
    }

    const maxFront = 50;
    const minFront = 0;
    const vehicleHalfLength = 7.5;
    const clampedFront = Math.min(maxFront, Math.max(minFront, frontDistance));

    return -((clampedFront + vehicleHalfLength) / 50) * 100;
}

function vehicleLeftFromRear(rearDistance) {
    if (rearDistance !== null && rearDistance !== undefined) {
        const maxRear = 80;
        const minRear = 4;
        const slotLength = 50;
        const vehicleHalfLength = 7.5;
        const clampedRear = Math.min(maxRear, Math.max(minRear, rearDistance));
        const centerFromSlotFront = slotLength - clampedRear - vehicleHalfLength;

        return (centerFromSlotFront / slotLength) * 100;
    }

    return null;
}

function vehicleLeftFromDistance(state, distances) {
    if (state === "APPROACHING") {
        return vehicleLeftFromFront(distances.front);
    }

    return vehicleLeftFromRear(distances.rear);
}

function updateVehicle(state, distances) {
    const vehicle = document.getElementById("vehicle");
    vehicle.className = "vehicle";
    vehicle.style.left = "";

    if (state === "EMPTY" || state === "UNKNOWN") {
        return;
    }

    vehicle.classList.add("active");

    const measuredLeft = vehicleLeftFromDistance(state, distances);
    if (measuredLeft !== null) {
        vehicle.style.left = `${measuredLeft}%`;
    }

    if (state === "APPROACHING") {
        vehicle.classList.add("approaching");
    } else if (state === "PARKING") {
        vehicle.classList.add("parking");
    } else {
        vehicle.classList.add("parked");
    }
}

function updateGate(state) {
    const gate = document.getElementById("gate");
    gate.className = "gate";

    if (state === "OPEN") {
        gate.classList.add("open");
    }
}

function updateEvents(events) {
    const list = document.getElementById("events");
    list.innerHTML = "";

    events.forEach((event) => {
        const item = document.createElement("li");
        item.textContent = `${event.timestamp} | ${event.topic} | ${event.event_type}`;
        list.appendChild(item);
    });
}

async function loadState() {
    const response = await fetch("/api/state");
    const state = await response.json();

    setText("parking-state", state.parking_state);
    setText("risk-state", state.risk_state);
    setText("gate-state", state.gate_state);
    setText("front-distance", valueOrDash(state.distances.front, "cm"));
    setText("rear-distance", valueOrDash(state.distances.rear, "cm"));
    setText("side-distance", valueOrDash(state.distances.side, "cm"));
    setText("last-updated", valueOrDash(state.last_updated));

    updateVehicle(state.parking_state, state.distances);
    updateGate(state.gate_state);
    updateEvents(state.events || []);
}

loadState();
setInterval(loadState, 200);
