const MQTT_WS_PORT = 9001;
const MQTT_TOPICS = [
    "parking/#",
];

let recentEvents = [];
let dashboardState = {
    slot: {
        slot1: null,
        slot2: null,
    },
    distance: {
        slot1: null,
        slot2: null,
    },
    lot_state: null,
    gate_state: null,
};

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

function toClassName(value) {
    return String(value || "unknown").toLowerCase().replace(/\s+/g, "-");
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

    element.classList.add(toClassName(state));
}

function updateStatePill(id, state) {
    const element = document.getElementById(id);
    if (!element) {
        return;
    }

    const normalizedState = textOrUnknown(state);
    element.textContent = normalizedState;
    element.className = "state-pill";
    element.classList.add(toClassName(normalizedState));
}

function updateAvailableSlots() {
    const states = [
        dashboardState.slot.slot1,
        dashboardState.slot.slot2,
    ];
    const knownStates = states.filter((state) => state);

    if (knownStates.length === 0) {
        setText("available-slots", "-");
        return;
    }

    setText("available-slots", String(states.filter((state) => state === "EMPTY").length));
}

function updateSlot(slotId, state, distance) {
    const slotElement = document.getElementById(slotId);
    const normalizedState = textOrUnknown(state);

    setStateClass(slotElement, "data-card slot-card", normalizedState);

    updateStatePill(`${slotId}-state`, normalizedState);
    setText(`${slotId}-distance`, formatDistance(distance));
    setText(`metric-${slotId}-distance`, formatDistance(distance));
    updateAvailableSlots();
}

function updateLotState(state) {
    const normalizedState = textOrUnknown(state);
    setText("lot-state", normalizedState);

    const lotElement = document.getElementById("lot-state");
    if (!lotElement) {
        return;
    }

    lotElement.className = "";
    lotElement.classList.add(toClassName(normalizedState));
}

function updateGate(state) {
    const normalizedState = textOrUnknown(state);
    setText("gate-state", normalizedState);
    setText("gate-state-detail", normalizedState);

    const gateElement = document.getElementById("gate");
    setStateClass(gateElement, "data-card gate-card", normalizedState);
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

function appendEvent(event) {
    recentEvents = [event, ...recentEvents].slice(0, 10);
    updateEvents(recentEvents);
}

function setMqttStatus(status) {
    const normalizedStatus = textOrUnknown(status);
    const element = document.getElementById("mqtt-status");

    if (!element) {
        return;
    }

    element.textContent = normalizedStatus;
    element.className = toClassName(normalizedStatus);
}

function updateLastUpdated() {
    setText("last-updated", new Date().toLocaleString());
}

function updateLastMqttMessage(topic, payload) {
    setText("last-mqtt-topic", topic);
    setText("last-mqtt-payload", payload);
}

function updateLastGateEvent(eventType, reason) {
    setText("last-gate-event", textOrUnknown(eventType));
    setText("last-gate-reason", reason || "-");
}

function applyState(data) {
    dashboardState = {
        slot: {
            ...dashboardState.slot,
            ...(data.slot || {}),
        },
        distance: {
            ...dashboardState.distance,
            ...(data.distance || {}),
        },
        lot_state: data.lot_state || dashboardState.lot_state,
        gate_state: data.gate_state || dashboardState.gate_state,
    };

    updateSlot("slot1", dashboardState.slot.slot1, dashboardState.distance.slot1);
    updateSlot("slot2", dashboardState.slot.slot2, dashboardState.distance.slot2);
    updateLotState(dashboardState.lot_state);
    updateGate(dashboardState.gate_state);

    setText("last-updated", data.last_updated || "-");
    recentEvents = data.events || [];
    updateEvents(recentEvents);

    if (recentEvents.length > 0) {
        updateLastGateEvent(recentEvents[0].event_type, recentEvents[0].reason);
    }
}

function handleMqttMessage(topic, payload) {
    if (topic === "parking/rpi1/slot") {
        const data = JSON.parse(payload);
        dashboardState.slot = {
            ...dashboardState.slot,
            slot1: data.slot1,
            slot2: data.slot2,
        };
        updateSlot("slot1", dashboardState.slot.slot1, dashboardState.distance.slot1);
        updateSlot("slot2", dashboardState.slot.slot2, dashboardState.distance.slot2);
        updateLastUpdated();
        return;
    }

    if (topic === "parking/rpi1/distance") {
        const data = JSON.parse(payload);
        dashboardState.distance = {
            ...dashboardState.distance,
            slot1: data.slot1,
            slot2: data.slot2,
        };
        updateSlot("slot1", dashboardState.slot.slot1, dashboardState.distance.slot1);
        updateSlot("slot2", dashboardState.slot.slot2, dashboardState.distance.slot2);
        updateLastUpdated();
        return;
    }

    if (topic === "parking/rpi1/lot") {
        dashboardState.lot_state = payload;
        updateLotState(payload);
        updateLastUpdated();
        return;
    }

    if (topic === "parking/rpi2/gate") {
        dashboardState.gate_state = payload;
        updateGate(payload);
        updateLastUpdated();
        return;
    }

    if (topic === "parking/rpi2/event") {
        const data = JSON.parse(payload);
        updateLastGateEvent(data.event, data.reason);
        appendEvent({
            timestamp: new Date().toLocaleString(),
            topic,
            event_type: data.event,
            reason: data.reason,
        });
        updateLastUpdated();
    }
}

async function fetchState() {
    try {
        const response = await fetch("/api/state", {
            cache: "no-store",
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        applyState(await response.json());
    } catch (error) {
        console.error("Failed to load initial dashboard state:", error);
    }
}

function connectMqtt() {
    if (!window.mqtt) {
        setMqttStatus("MQTT.JS NOT LOADED");
        return;
    }

    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const host = window.location.hostname || "localhost";
    const client = mqtt.connect(`${protocol}://${host}:${MQTT_WS_PORT}`);

    client.on("connect", () => {
        setMqttStatus("CONNECTED");
        client.subscribe(MQTT_TOPICS, (error) => {
            if (error) {
                setMqttStatus("SUBSCRIBE FAILED");
                console.error("MQTT subscribe failed:", error);
                return;
            }

            setMqttStatus("SUBSCRIBED");
        });
    });

    client.on("message", (topic, message) => {
        const payload = message.toString();
        console.log("MQTT message received:", topic, payload);
        updateLastMqttMessage(topic, payload);

        try {
            handleMqttMessage(topic, payload);
        } catch (error) {
            console.error("Failed to handle MQTT message:", topic, error);
            setMqttStatus("MESSAGE ERROR");
        }
    });

    client.on("reconnect", () => {
        setMqttStatus("RECONNECTING");
    });

    client.on("close", () => {
        setMqttStatus("DISCONNECTED");
    });

    client.on("error", (error) => {
        setMqttStatus("ERROR");
        console.error("MQTT connection error:", error);
    });
}

fetchState();
connectMqtt();
