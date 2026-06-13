const fs = require('fs');
const zlib = require('zlib');

let html = fs.readFileSync('src/index.html', 'utf8');

// Replace script
let sjs = fs.readFileSync('src/s.js', 'utf8');
html = html.replace('<script src="s.js"></script>', '<script>\n' + sjs + '\n</script>');

let gzipped = zlib.gzipSync(html);

let out = '#define index_html_gz_len ' + gzipped.length + '\n';
out += 'static const uint8_t index_html_gz[] PROGMEM = {';
for (let i = 0; i < gzipped.length; i++) {
    out += gzipped[i];
    if (i < gzipped.length - 1) out += ',';
}
out += '};';

fs.writeFileSync('index.html.gz.h', out);
console.log('Build successful, length: ' + gzipped.length);
