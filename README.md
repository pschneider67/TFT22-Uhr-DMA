# TFT22-Uhr-DMA
Die Software läuft auf der Platine TFT22-Uhr. Die KiCad Daten der Platine sind im Repository TFT-Uhr.

Kurze Beschreibung des Weckers

Bedienelemente    
Taster 1 - Menüumschaltung <br>
Der Taster muss so lange gedrückt werden bis in der Statuszeile (unten) ein neuer Menüpunkt angezeigt wird.
Nun kann man ducrh die Menüs achalten.<br><br>
Taster 2     
Mit diesem Taster wird das jeweilige Menü gestartet. Die Taste muss so lange gedrückt werden bis eine Aktion sichtbar wird.
<br><br>
Die Menüs werden nach einiger Zeit selbständig verlassen. Sind neue Konfigurationsdaten entstanden, so werden diese automatisch im Flash des ESP8266 gespeichert. 
Dies wird durch einen kurzen Piep signalisiert. Ein Stern vor der Weckzeit zeigt an das der Wecker für diese Zeit aktiv ist. Das Aktivieren der Weckfunktion wird mit einem langen Druck auf die Taste 2 aktiviert. Die Displayhelligkeit wird über einen Fotowiderstand eingstellt und passt sich der Umgebungshelligkeit an. <br>
Die Anmeldung im WLAN erfolgt über die Funktionen des WifiManger. Der ESP8266 stellt einen AP bereit über den die Zugangsdaten zum WLAN konfiguriert werden können. Die Daten werden im Flash des ESP8266 gespeichert und stehen so beim Neustart bereit. 
