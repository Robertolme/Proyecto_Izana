const express = require('express');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const port = 3000;

// Inicializas el puerto serie al mismo baudrate que tu sketch Arduino/ESP32
const uart = new SerialPort({
  path: '/dev/ttyUSB0',
  baudRate: 115200,
  autoOpen: true
});

// Creamos un parser de línea que divide en '\n'
const parser = uart.pipe(new ReadlineParser({ delimiter: '\n' }));

// Evento cuando llega una línea completa desde el ESP32
parser.on('data', (line) => {
  // line tendrá algo como "signal:123"
  console.log('Dato recibido del ESP:', line);

  // Si quieres extraer sólo el número:
  const m = line.match(/^signal:(\d{1,3})$/);
  if (m) {
    const valor = parseInt(m[1], 10);
    console.log('Valor numérico:', valor);
    // Aquí podrías emitir por WebSocket, Guardar en DB, etc.
  }
});

// Manejo de apertura / errores
uart.on('open', () => console.log('UART abierto a 115200bps'));
uart.on('error', err => console.error('Error UART:', err.message));

app.use(express.static('public'));
app.use(express.json());
// ... tu ruta POST /color aquí ...
app.listen(port, () => {
  console.log(`Servidor corriendo en http://localhost:${port}`);
});
