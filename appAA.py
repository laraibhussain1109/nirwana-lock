from http.server import BaseHTTPRequestHandler, HTTPServer

html = """

<!DOCTYPE html>
<html>

<head>

<meta name='viewport'
content='width=device-width, initial-scale=1'>

<title>SMART GATE</title>

<style>

body{
background:#111;
font-family:Arial;
text-align:center;
color:white;
margin:0;
padding:0;
}

.box{
background:#222;
padding:20px;
margin:20px;
border-radius:20px;
}

button{
padding:15px 25px;
font-size:18px;
border:none;
border-radius:12px;
margin:10px;
color:white;
cursor:pointer;
}

.green{
background:green;
}

.red{
background:red;
}

.keypad{
display:grid;
grid-template-columns:repeat(3,80px);
gap:10px;
justify-content:center;
margin-top:20px;
}

.key{
background:#333;
padding:20px;
border-radius:12px;
font-size:22px;
cursor:pointer;
}

.passbox{
font-size:28px;
letter-spacing:5px;
margin:20px;
min-height:40px;
}

input{
padding:12px;
width:220px;
border:none;
border-radius:10px;
font-size:18px;
text-align:center;
}

</style>

</head>

<body>

<h1>ESP32 SMART GATE</h1>

<div class='box'>

<h2>Gate Status</h2>

<h1 id='gate'>
Loading...
</h1>

<button class='green'
onclick='openGate()'>
OPEN
</button>

<button class='red'
onclick='closeGate()'>
CLOSE
</button>

</div>

<div class='box'>

<h2>Current Password</h2>

<h1 id='currentpass'>
------
</h1>

</div>

<div class='box'>

<h2>Fingerprint Status</h2>

<h1 id='finger'>
READY
</h1>

</div>

<div class='box'>

<h2>New Password</h2>

<div class='passbox'
id='pass'>
</div>

<div class='keypad'>

<div class='key'
onclick="addKey('1')">1</div>

<div class='key'
onclick="addKey('2')">2</div>

<div class='key'
onclick="addKey('3')">3</div>

<div class='key'
onclick="addKey('4')">4</div>

<div class='key'
onclick="addKey('5')">5</div>

<div class='key'
onclick="addKey('6')">6</div>

<div class='key'
onclick="addKey('7')">7</div>

<div class='key'
onclick="addKey('8')">8</div>

<div class='key'
onclick="addKey('9')">9</div>

<div class='key'
onclick="clearPass()">
CLR
</div>

<div class='key'
onclick="addKey('0')">0</div>

<div class='key'
onclick="changePassword()">
OK
</div>

</div>

</div>

<div class='box'>

<h2>Fingerprint</h2>

<input type='number'
id='fid'
placeholder='Finger ID'>

<br><br>

<button class='green'
onclick='enrollFinger()'>
ENROLL
</button>

<button class='red'
onclick='deleteFinger()'>
DELETE
</button>

</div>

<script>

let esp = "192.168.1.33";

let newpass = "";

// ================= STATUS =================

async function loadStatus(){

try{

let r = await fetch(
`http://${esp}/api/status`
);

let d = await r.json();

document.getElementById('gate').innerHTML =
d.gate;

document.getElementById('currentpass').innerHTML =
d.password;

document.getElementById('finger').innerHTML =
d.finger;

}catch(e){

document.getElementById('gate').innerHTML =
"ESP OFFLINE";

}
}

// ================= OPEN =================

async function openGate(){

await fetch(
`http://${esp}/api/open`
);

loadStatus();
}

// ================= CLOSE =================

async function closeGate(){

await fetch(
`http://${esp}/api/close`
);

loadStatus();
}

// ================= PASSWORD =================

function addKey(k){

if(newpass.length < 6){

newpass += k;

document.getElementById('pass').innerHTML =
newpass;

}
}

// ================= CLEAR =================

function clearPass(){

newpass = "";

document.getElementById('pass').innerHTML =
"";
}

// ================= CHANGE PASSWORD =================

async function changePassword(){

if(newpass.length != 6){

alert("6 DIGIT PASSWORD");

return;
}

await fetch(
`http://${esp}/api/password?pass=${newpass}`
);

alert("PASSWORD UPDATED");

clearPass();

loadStatus();
}

// ================= ENROLL =================

async function enrollFinger(){

document.getElementById('finger').innerHTML =
"WAITING";

let id =
document.getElementById('fid').value;

await fetch(
`http://${esp}/api/enroll?id=${id}`
);

loadStatus();
}

// ================= DELETE =================

async function deleteFinger(){

let id =
document.getElementById('fid').value;

await fetch(
`http://${esp}/api/delete?id=${id}`
);

loadStatus();
}

loadStatus();

setInterval(loadStatus, 1000);

</script>

</body>
</html>

"""

class MyServer(BaseHTTPRequestHandler):

    def do_GET(self):

        self.send_response(200)

        self.send_header(
        'Content-type',
        'text/html'
        )

        self.end_headers()

        self.wfile.write(
        html.encode()
        )

server = HTTPServer(
("0.0.0.0", 8000),
MyServer
)

print("SERVER STARTED")

print("OPEN:")

print("http://127.0.0.1:8000")

server.serve_forever()