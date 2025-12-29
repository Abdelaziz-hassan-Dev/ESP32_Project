// 1. إعدادات Firebase (نفس إعداداتك السابقة)
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

// دالة لإنشاء إعدادات الرسم بشكل موحد
function createChartConfig(label, color) {
    return {
        type: 'line',
        data: {
            labels: [], // الوقت
            datasets: [{
                label: label,
                data: [],
                borderColor: color,
                backgroundColor: color + '33', // شفافية
                borderWidth: 2,
                tension: 0.4, // نعومة الخط
                fill: true,
                pointRadius: 3
            }]
        },
        options: {
            responsive: true,
            scales: {
                x: { 
                    display: false // إخفاء محور الوقت لتوفير المساحة
                },
                y: {
                    grid: { color: 'rgba(255,255,255,0.1)' },
                    ticks: { color: '#ccc' }
                }
            },
            plugins: {
                legend: { labels: { color: '#fff' } }
            }
        }
    };
}

// إنشاء المخططات
const tempCtx = document.getElementById('tempChart').getContext('2d');
const humCtx = document.getElementById('humChart').getContext('2d');

const tempChart = new Chart(tempCtx, createChartConfig('Temperature History', '#ff8c00'));
const humChart = new Chart(humCtx, createChartConfig('Humidity History', '#00d2ff'));

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

// دالة تحديث المخطط (تضيف نقطة وتحذف القديمة)
function updateChart(chart, value) {
    const now = new Date().toLocaleTimeString();
    
    chart.data.labels.push(now);
    chart.data.datasets[0].data.push(value);

    // الاحتفاظ بآخر 20 قراءة فقط
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
    const now = new Date();
    document.getElementById("lastUpdate").innerText = now.toLocaleTimeString();

    // تغيير النقطة للأخضر
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    
    dot.className = "dot online";
    text.innerText = "Live";
}

// التحقق من حالة الاتصال بـ Firebase نفسه
const connectedRef = firebase.database().ref(".info/connected");
connectedRef.on("value", (snap) => {
  if (snap.val() === true) {
    console.log("Connected to Firebase");
  } else {
    // إذا انقطع النت كلياً
    document.getElementById("connectionDot").className = "dot offline";
    document.getElementById("connectionText").innerText = "Offline";
  }
});



// // 1. إعدادات مشروعك (تأخذها من Firebase Console -> Project Settings -> General -> CDN)
// // Your web app's Firebase configuration
// const firebaseConfig = {
//   apiKey: "AIzaSyDDdgCi-ZwiVJN9xIBd-BsopL8tWbnfZWo",
//   authDomain: "esp32-ce491.firebaseapp.com",
//   databaseURL: "https://esp32-ce491-default-rtdb.firebaseio.com",
//   projectId: "esp32-ce491",
//   storageBucket: "esp32-ce491.firebasestorage.app",
//   messagingSenderId: "1012960274280",
//   appId: "1:1012960274280:web:84a6c1800fb722cb6d58dd"
// };

// // 2. تهيئة Firebase
// firebase.initializeApp(firebaseConfig);
// const database = firebase.database();

// // 3. الاستماع للبيانات (Realtime Listener)
// // هذه الدالة تعمل تلقائياً في كل مرة يتغير فيها الرقم في الداتابيس
// database.ref('/sensor').on('value', (snapshot) => {
//     const data = snapshot.val();
    
//     if (data) {
//         // تحديث الحرارة
//         document.getElementById("temperature").innerText = data.temperature.toFixed(1);
        
//         // تحديث الرطوبة
//         document.getElementById("humidity").innerText = data.humidity.toFixed(1);
        
//         // تحديث الحريق
//         const flameStatus = data.flame;
//         const el = document.getElementById("flame");
//         const card = document.getElementById("flameCard");
        
//         el.innerText = flameStatus;
        
//         if(flameStatus === "DETECTED") {
//             card.classList.remove("flame-safe");
//             card.classList.add("flame-danger");
//             el.style.color = "#ff4444";
//         } else {
//             card.classList.remove("flame-danger");
//             card.classList.add("flame-safe");
//             el.style.color = "#00c851";
//         }
//     }
// });

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// function fetchData(url, elementId) {
//     var xhttp = new XMLHttpRequest();
//     xhttp.onreadystatechange = function() {
//         if (this.readyState == 4 && this.status == 200) {
//             document.getElementById(elementId).innerText = this.responseText;
//         }
//     };
//     xhttp.open("GET", url, true);
//     xhttp.send();
// }

// function updateFlame() {
//     var xhttp = new XMLHttpRequest();
//     xhttp.onreadystatechange = function() {
//         if (this.readyState == 4 && this.status == 200) {
//             var status = this.responseText;
//             var el = document.getElementById("flame");
//             var card = document.getElementById("flameCard");
            
//             el.innerText = status;

//             // تغيير التصميم بناء على الحالة
//             if(status.includes("DETECTED") || status.includes("FIRE")) {
//                 card.classList.remove("flame-safe");
//                 card.classList.add("flame-danger");
//                 el.style.color = "#ff4444";
//             } else {
//                 card.classList.remove("flame-danger");
//                 card.classList.add("flame-safe");
//                 el.style.color = "#00c851";
//             }
//         }
//     };
//     xhttp.open("GET", "/flame", true);
//     xhttp.send();
// }

// // تشغيل الدوال بشكل دوري
// setInterval(function() { fetchData("/temperature", "temperature"); }, 2000);
// setInterval(function() { fetchData("/humidity", "humidity"); }, 2000);
// setInterval(updateFlame, 1000); // تحديث الحريق كل ثانية