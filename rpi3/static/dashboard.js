const POLL_INTERVAL_MS = 500;

function textOrUnknown(value) {
    if (value === null || value === undefined || value === "") {
        return "UNKNOWN";
    }
    return value;
}

function formatDistance(value) {
    if (value === null || value === undefined) {
        return "-";
    }
    return `${value} cm`;
}

function setText(id, value) {
    const element = document.getElementById(id);
    if (!element) {
        return;
    }
    element.textContent = value;
}

function setStateClass(element, baseClass, state) {
    if (!element) {
        return;
    }

    element.className = baseClass;

    if (!state) {
        element.classList.add("unknown");
        return;
    }

    element.classList.add(String(state).toLowerCase());
}

function updateSlot(slotId, state, distance) {
    const slotElement = document.getElementById(slotId);
    const normalizedState = textOrUnknown(state);

    setStateClass(slotElement, "slot", normalizedState);

    setText(`${slotId}-state`, normalizedState);
    setText(`${slotId}-distance`, formatDistance(distance));
    setText(`metric-${slotId}-distance`, formatDistance(distance));
}

function updateLotState(state) {
    const normalizedState = textOrUnknown(state);
    setText("lot-state", normalizedState);

    const lotElement = document.getElementById("lot-state");
    if (!lotElement) {
        return;
    }

    lotElement.className = "";
    lotElement.classList.add(normalizedState.toLowerCase());
}

function updateGate(state) {
    const normalizedState = textOrUnknown(state);
    setText("gate-state", normalizedState);

    const gateElement = document.getElementById("gate");
    setStateClass(gateElement, "gate", normalizedState);
}

function updateEvents(events) {
    const eventList = document.getElementById("events");
    if (!eventList) {
        return;
    }

    eventList.innerHTML = "";

    if (!events || events.length === 0) {
        const item = document.createElement("li");
        item.textContent = "No events";
        eventList.appendChild(item);
        return;
    }

    events.forEach((event) => {
        const item = document.createElement("li");

        const timestamp = event.timestamp || "-";
        const eventType = event.event_type || "unknown_event";
        const reason = event.reason ? ` (${event.reason})` : "";
        const topic = event.topic || "";

        item.textContent = `[${timestamp}] ${eventType}${reason} ${topic}`;
        eventList.appendChild(item);
    });
}

async function fetchState() {
    try {
        const response = await fetch("/api/state", {
            cache: "no-store",
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        const data = await response.json();

        const slot = data.slot || {};
        const distance = data.distance || {};

        updateSlot("slot1", slot.slot1, distance.slot1);
        updateSlot("slot2", slot.slot2, distance.slot2);
        updateLotState(data.lot_state);
        updateGate(data.gate_state);

        setText("last-updated", data.last_updated || "-");
        updateEvents(data.events || []);
    } catch (error) {
        console.error("Failed to fetch dashboard state:", error);
    }
}

fetchState();
setInterval(fetchState, POLL_INTERVAL_MS);