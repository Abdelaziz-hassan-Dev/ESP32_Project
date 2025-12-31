// 1. إعدادات Firebase
const firebaseConfig = {
    apiKey: "AIzaSyDDdgCi-ZwiVJN9xIBd-BsopL8tWbnfZWo",
    authDomain: "esp32-ce491.firebaseapp.com",
    databaseURL: "https://esp32-ce491-default-rtdb.firebaseio.com",
    projectId: "esp32-ce491",
    storageBucket: "esp32-ce491.firebasestorage.app",
    messagingSenderId: "1012960274280",
    appId: "1:1012960274280:web:84a6c1800fb722cb6d58dd"
};
  
// تهيئة Firebase
firebase.initializeApp(firebaseConfig);
const database = firebase.database();

// ================= إعدادات الرسوم البيانية (Charts) =================

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
                    // تحسين التلميح (Tooltip) ليظهر القيمة بوضوح عند تمرير الماوس
                    mode: 'index',
                    intersect: false,
                }
            }
        }
    };
}

// إنشاء المخططات
const tempCtx = document.getElementById('tempChart').getContext('2d');
const humCtx = document.getElementById('humChart').getContext('2d');

const tempChart = new Chart(tempCtx, createChartConfig('Temperature History', '#ff8c00', 10, 30));
const humChart = new Chart(humCtx, createChartConfig('Humidity History', '#00d2ff', 30, 80));

// ================= استلام البيانات =================

// الاستماع للبيانات من Firebase
database.ref('/sensor').on('value', (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // 1. تحديث النصوص (Cards)
        document.getElementById("temperature").innerText = data.temperature.toFixed(1);
        document.getElementById("humidity").innerText = data.humidity.toFixed(1);
        
        // 2. تحديث الحريق
        updateFlameStatus(data.flame);

        // 3. تحديث الرسوم البيانية
        updateChart(tempChart, data.temperature);
        updateChart(humChart, data.humidity);

        // 4. تحديث التوقيت وحالة الاتصال
        updateConnectionStatus();
    }
});

// دالة مساعدة للحصول على الوقت بصيغة (HH:MM) فقط
function getCurrentTimeShort() {
    const now = new Date();
    // هذه الدالة تعيد الوقت بدون ثواني (مثلاً 10:30 PM)
    return now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

// دالة تحديث المخطط
function updateChart(chart, value) {
    // نستخدم الدالة الجديدة لجلب الوقت المختصر
    const timeString = getCurrentTimeShort();
    
    chart.data.labels.push(timeString);
    chart.data.datasets[0].data.push(value);

    // الاحتفاظ بآخر 20 قراءة
    if(chart.data.labels.length > 20) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
    }
    chart.update();
}

// دالة معالجة حالة الحريق
function updateFlameStatus(status) {
    const el = document.getElementById("flame");
    const card = document.getElementById("flameCard");
    
    el.innerText = status;
    
    if(status === "DETECTED") {
        card.classList.remove("flame-safe");
        card.classList.add("flame-danger");
        el.innerText = "DANGER! 🔥";
        el.style.color = "#ff4444";
    } else {
        card.classList.remove("flame-danger");
        card.classList.add("flame-safe");
        el.innerText = "Safe ✅";
        el.style.color = "#00c851";
    }
}

// دالة تحديث حالة الاتصال والوقت
function updateConnectionStatus() {
    // نستخدم نفس تنسيق الوقت المبسط هنا أيضاً
    document.getElementById("lastUpdate").innerText = getCurrentTimeShort();

    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    
    dot.className = "dot online";
    text.innerText = "Live";
}

// التحقق من حالة الاتصال بـ Firebase
const connectedRef = firebase.database().ref(".info/connected");
connectedRef.on("value", (snap) => {
  if (snap.val() === true) {
    console.log("Connected to Firebase");
  } else {
    document.getElementById("connectionDot").className = "dot offline";
    document.getElementById("connectionText").innerText = "Offline";
  }
});