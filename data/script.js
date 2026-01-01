// 1. Firebase Configuration
const firebaseConfig = {
    apiKey: "AIzaSyDDdgCi-ZwiVJN9xIBd-BsopL8tWbnfZWo",
    authDomain: "esp32-ce491.firebaseapp.com",
    databaseURL: "https://esp32-ce491-default-rtdb.firebaseio.com",
    projectId: "esp32-ce491",
    storageBucket: "esp32-ce491.firebasestorage.app",
    messagingSenderId: "1012960274280",
    appId: "1:1012960274280:web:84a6c1800fb722cb6d58dd"
};
  
// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const database = firebase.database();

// ================= Chart Configuration =================

function createChartConfig(label, color, minVal, maxVal) {
    return {
        type: 'line',
        data: {
            labels: [], 
            datasets: [{
                label: label,
                data: [],
                borderColor: color,
                backgroundColor: color + '33',
                borderWidth: 2,
                tension: 0.4,
                fill: true,
                pointRadius: 3
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false, 
            scales: {
                x: { 
                    display: true,
                    grid: { color: 'rgba(255,255,255,0.05)' },
                    ticks: { 
                        color: '#aaa',
                        maxTicksLimit: 6,
                        maxRotation: 0 
                    }
                },
                y: {
                    suggestedMin: minVal, 
                    suggestedMax: maxVal,
                    grid: { color: 'rgba(255,255,255,0.1)' },
                    ticks: { color: '#ccc' }
                }
            },
            plugins: {
                legend: { labels: { color: '#fff' } },
                tooltip: {
                    mode: 'index',
                    intersect: false,
                }
            }
        }
    };
}

// Chart Initialization
const tempCtx = document.getElementById('tempChart').getContext('2d');
const humCtx = document.getElementById('humChart').getContext('2d');

const tempChart = new Chart(tempCtx, createChartConfig('Temperature History', '#ff8c00', 10, 30));
const humChart = new Chart(humCtx, createChartConfig('Humidity History', '#00d2ff', 30, 80));

// ================= Data Listener & Watchdog Logic =================

let watchdogTimer;

// Main Firebase Listener (Real-time updates)
database.ref('/sensor').on('value', (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // 1. Data received indicates system is Online
        showOnlineStatus();

        // 2. Update UI elements
        document.getElementById("temperature").innerText = data.temperature.toFixed(1);
        document.getElementById("humidity").innerText = data.humidity.toFixed(1);
        
        updateFlameStatus(data.flame);
        updateChart(tempChart, data.temperature);
        updateChart(humChart, data.humidity);
        
        // 3. Watchdog Reset
        // Clear previous timer as we just received a heartbeat
        clearTimeout(watchdogTimer);

        // Set new timeout: if no data for 15s, mark system as Offline
        watchdogTimer = setTimeout(showOfflineStatus, 15000); 
    }
});

// UI State: Online
function showOnlineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    
    dot.className = "dot online";
    text.innerText = "Live";
    
    // Restore opacity
    document.getElementById("temperature").style.opacity = "1";
    document.getElementById("humidity").style.opacity = "1";
    
    document.getElementById("lastUpdate").innerText = getCurrentTimeShort();
}

// UI State: Offline
function showOfflineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    const flameText = document.getElementById("flame");

    dot.className = "dot offline";
    text.innerText = "Offline";
    
    // Dim values to indicate stale data
    document.getElementById("temperature").style.opacity = "0.4";
    document.getElementById("humidity").style.opacity = "0.4";
    
    // Update status indicators
    flameText.innerText = "No Signal";
    flameText.style.color = "gray";
    document.getElementById("flameCard").className = "card flame-card"; 
}

// ================= Helper Functions =================

// Returns time in HH:MM format
function getCurrentTimeShort() {
    const now = new Date();
    return now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function updateChart(chart, value) {
    const timeString = getCurrentTimeShort();
    
    chart.data.labels.push(timeString);
    chart.data.datasets[0].data.push(value);

    // Keep last 20 data points (Rolling window)
    if(chart.data.labels.length > 20) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
    }
    chart.update();
}

function updateFlameStatus(status) {
    const el = document.getElementById("flame");
    const card = document.getElementById("flameCard");
    
    el.innerText = status;
    
    if(status === "DETECTED") {
        card.classList.remove("flame-safe");
        card.classList.add("flame-danger");
        el.innerText = "DANGER! ⚠️";
        el.style.color = "#ff4444";
    } else {
        card.classList.remove("flame-danger");
        card.classList.add("flame-safe");
        el.innerText = "Safe ✅";
        el.style.color = "#00c851";
    }
}

// Browser connection status (Debug only)
const connectedRef = firebase.database().ref(".info/connected");
connectedRef.on("value", (snap) => {
  if (snap.val() === true) {
    console.log("Browser connected to Firebase");
  } else {
    console.log("Browser disconnected from Firebase");
  }
});