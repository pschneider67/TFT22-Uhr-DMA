/*
 * html.h
 *
 *  Created on: 16.03.2023
 *      Author: pschneider
 */

/*
const char cHtmlMessage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
	<title>ESP8266 Server Values</title>
	<script>
		window.addEventListener("load", function() {
			var xhr = new XMLHttpRequest();
			xhr.onreadystatechange = function() {
				if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
					var response = xhr.responseText;
					var values = response.split(",");
					document.getElementById("temperature").innerHTML = values[0];
					document.getElementById("humidity").innerHTML = values[1];
					document.getElementById("light").innerHTML = values[2];
				}
			};
			xhr.open("GET", "/values");
			xhr.send();
		});
	</script>
</head>
<body>
	<h1>ESP8266 Server Values</h1>
	
	<!-- Values from ESP8266 Server -->
	<p>Temperature: <span id="temperature"></span></p>
	<p>Humidity: <span id="humidity"></span></p>
	<p>Light: <span id="light"></span></p>
</body>
</html>
)=====";


const char cHtmlMessage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
	<title>ESP8266 Server Values</title>
		<script>
		window.addEventListener("load", function() {
			var xhr = new XMLHttpRequest();
			xhr.onreadystatechange = function() {
				if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
					var response = xhr.responseText;
					var values = response.split(",");
					document.getElementById("temperature").innerHTML = values[0];
					document.getElementById("humidity").innerHTML = values[1];
					document.getElementById("light").innerHTML = values[2];
				}
			};
			xhr.open("GET", "/values");
			xhr.send();
		});
	</script>
</head>
<body>
	<h1>ESP8266 Server Values</h1>
	
	<!-- Values from ESP8266 Server -->
	<p>Temperature: <span id="temperature"></span></p>
	<p>Humidity: <span id="humidity"></span></p>
	<p>Light: <span id="light"></span></p>
</body>
</html>
)=====";
*/

const char cHtmlMessage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
    <title>ESP8266 Wecker</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">

	<script>
		window.addEventListener("load", function() {
			var xhr = new XMLHttpRequest();
			xhr.onreadystatechange = function() {
				if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
					var response = xhr.responseText;
					console.log(response);
					var ServerData = response.split(",");

					document.getElementById("httpWz0").value = ServerData[0];	
					document.getElementById("httpTage0").value = ServerData[1];
					if (ServerData[2] == "1") {
						document.getElementById("httpAktiv0").checked = true;
					} else {
						document.getElementById("httpAktiv0").checked = false;
					}

					document.getElementById("httpWz1").value = ServerData[3];	
					document.getElementById("httpTage1").value = ServerData[4];
					if (ServerData[5] == "1") {
						document.getElementById("httpAktiv1").checked = true;
					} else {
						document.getElementById("httpAktiv1").checked = false;
					}
					
					document.getElementById("httpWz2").value = ServerData[6];	
					document.getElementById("httpTage2").value = ServerData[7];
					if (ServerData[8] == "1") {
						document.getElementById("httpAktiv2").checked = true;
					} else {
						document.getElementById("httpAktiv2").checked = false;
					}

					document.getElementById("httpWz3").value = ServerData[9];	
					document.getElementById("httpTage3").value = ServerData[10];
					if (ServerData[11] == "1") {
						document.getElementById("httpAktiv3").checked = true;
					} else {
						document.getElementById("httpAktiv3").checked = false;
					}

					document.getElementById("httpWz4").value = ServerData[12];	
					document.getElementById("httpTage4").value = ServerData[13];
					if (ServerData[14] == "1") {
						document.getElementById("httpAktiv4").checked = true;
					} else {
						document.getElementById("httpAktiv4").checked = false;
					}

					document.getElementById("httpWz5").value = ServerData[15];	
					document.getElementById("httpTage5").value = ServerData[16];
					if (ServerData[17] == "1") {
						document.getElementById("httpAktiv5").checked = true;
					} else {
						document.getElementById("httpAktiv5").checked = false;
					}

					document.getElementById("httpWz6").value = ServerData[18];	
					document.getElementById("httpTage6").value = ServerData[19];
					if (ServerData[20] == "1") {
						document.getElementById("httpAktiv6").checked = true;
					} else {
						document.getElementById("httpAktiv6").checked = false;
					}
				}
			};
			xhr.open("GET", "/values");
			xhr.send();
		});
	</script>
</head>

<body><h1>ESP8266 Wecker</h1>

<form method="post" action="/config">

<table>
	<tr>
		<td>Weckzeit 0</td>
		<td><input type="checkbox" id="httpAktiv0" name="httpAktiv0"></td>
		<td><input type="time" id="httpWz0" name="httpWz0"></td>
		<td><select name="httpTage0" id="httpTage0" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB0">Zeit 0 senden</button></td>
	</tr>

	<tr>		
		<td>Weckzeit 1</td>
		<td><input type="checkbox" id="httpAktiv1" name="httpAktiv1"></td>
		<td><input type="time" id="httpWz1" name="httpWz1"></td>
		<td><select name="httpTage1" id="httpTage1" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB1">Zeit 1 senden</button></td>
	</tr>

	<tr>
		<td>Weckzeit 2</td>
		<td><input type="checkbox" id="httpAktiv2" name="httpAktiv2"></td>
		<td><input type="time" id="httpWz2" name="httpWz2"></td>
		<td><select name="httpTage2" id="httpTage2" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB2">Zeit 2 senden</button></td>
	</tr>

	<tr>
		<td>Weckzeit 3</td>
		<td><input type="checkbox" id="httpAktiv3" name="httpAktiv3"></td>
		<td><input type="time" id="httpWz3" name="httpWz3"></td>
		<td><select name="httpTage3" id="httpTage3" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB3">Zeit 3 senden</button></td>
	</tr>

	<tr>
		<td>Weckzeit 4</td>
		<td><input type="checkbox" id="httpAktiv4" name="httpAktiv4"></td>
		<td><input type="time" id="httpWz4" name="httpWz4"></td>
		<td><select name="httpTage4" id="httpTage4" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB4">Zeit 4 senden</button></td>
	</tr>

	<tr>
		<td>Weckzeit 5</td>
		<td><input type="checkbox" id="httpAktiv5" name="httpAktiv5"></td>
		<td><input type="time" id="httpWz5" name="httpWz5"></td>
		<td><select name="httpTage5" id="httpTage5" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB5">Zeit 5 senden</button></td>
	</tr>

	<tr>
		<td>Weckzeit 6</td>
		<td><input type="checkbox" id="httpAktiv6" name="httpAktiv6"></td>
		<td><input type="time" id="httpWz6" name="httpWz6"></td>
		<td><select name="httpTage6" id="httpTage6" size="1">
			<option value=0>Sonntag</option>
			<option value=1>Montag</option>
			<option value=2>Dienstag</option>
			<option value=3>Mittwoch</option>
			<option value=4>Donnerstag</option>
			<option value=5>Freitag</option>
			<option value=6>Samstag</option>
			<option value=7>Jeden Tag</option>
			<option value=8>Mo. bis Fr.</option>
			<option value=9>Wochenende</option>
		</select></td>
		<td><button name="httpB6">Zeit 6 senden</button></td>
	</tr>
</table>
<br>
<button name="httpSaveAll">Alle Zeiten speichern</button>
</form>

<form method="post" action="/delete">
<br>
<button name="httpReset">Alle Zeiten löschen</button>
</form>

</body>
</html>
)=====";

const char cHtmlDelete[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
    <title>ESP8266 Wecker</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body><h1>ESP8266 Wecker</h1>
<form method="post" action="/">
Zeiten wurden gelöscht<br><br>
<button name="httpBack">zurück</button>
</form>

</body>
</html>
)=====";

const char cHtmlSave[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
    <title>ESP8266 Wecker</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body><h1>ESP8266 Wecker</h1>
<form method="post" action="/">
Neue Zeiten wurden gespichert<br><br>
<button name="httpBack">zurück</button>
</form>

</body>
</html>
)=====";