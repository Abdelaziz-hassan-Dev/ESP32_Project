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

// ================= استلام البيانات والتحقق من الاتصال (Watchdog) =================

// متغير للمؤقت (الحارس)
let watchdogTimer;

// الاستماع للبيانات من Firebase
database.ref('/sensor').on('value', (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // 1. لقد وصلت بيانات! إذن نحن Online
        showOnlineStatus();

        // 2. تحديث الواجهة بالبيانات الحية
        document.getElementById("temperature").innerText = data.temperature.toFixed(1);
        document.getElementById("humidity").innerText = data.humidity.toFixed(1);
        
        // تحديث الحريق والمخططات
        updateFlameStatus(data.flame);
        updateChart(tempChart, data.temperature);
        updateChart(humChart, data.humidity);
        
        // 3. (خدعة المؤقت)
        // قم بإلغاء المؤقت السابق لأننا استلمنا بيانات جديدة للتو
        clearTimeout(watchdogTimer);

        // ابدأ مؤقت جديد: إذا لم تصل بيانات أخرى خلال 6 ثواني، نفذ دالة showOfflineStatus
        watchdogTimer = setTimeout(showOfflineStatus, 9000); 
    }
});

// دالة لإظهار أننا متصلون
function showOnlineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    
    dot.className = "dot online";
    text.innerText = "Live";
    
    // إعادة الألوان لطبيعتها
    document.getElementById("temperature").style.opacity = "1";
    document.getElementById("humidity").style.opacity = "1";
    
    // تحديث التوقيت
    document.getElementById("lastUpdate").innerText = getCurrentTimeShort();
}

// دالة لإظهار أننا فقدنا الاتصال
function showOfflineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    const flameText = document.getElementById("flame");

    dot.className = "dot offline";
    text.innerText = "Offline";
    
    // جعل الأرقام باهتة للدلالة على أنها قديمة
    document.getElementById("temperature").style.opacity = "0.4";
    document.getElementById("humidity").style.opacity = "0.4";
    
    // تحذير في خانة الحريق
    flameText.innerText = "No Signal";
    flameText.style.color = "gray";
    document.getElementById("flameCard").className = "card flame-card"; // إزالة اللون الأخضر أو الأحمر
}

// ================= دوال مساعدة =================

// دالة مساعدة للحصول على الوقت بصيغة (HH:MM) فقط
function getCurrentTimeShort() {
    const now = new Date();
    return now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

// دالة تحديث المخطط
function updateChart(chart, value) {
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
        el.innerText = "DANGER! ⚠️";
        el.style.color = "#ff4444";
    } else {
        card.classList.remove("flame-danger");
        card.classList.add("flame-safe");
        el.innerText = "Safe ✅";
        el.style.color = "#00c851";
    }
}

// التحقق من حالة اتصال المتصفح بالإنترنت (للتنقيح فقط)
const connectedRef = firebase.database().ref(".info/connected");
connectedRef.on("value", (snap) => {
  if (snap.val() === true) {
    console.log("Browser connected to Firebase");
  } else {
    console.log("Browser disconnected from Firebase");
  }
});