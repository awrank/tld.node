var fs = require('fs');
var tld_module = require('tld.node');


var active_tld = fs.readFileSync("base.dat");
var reserved_tld  = fs.readFileSync("guide.dat");

tld_module.load(active_tld, reserved_tld);


var r = tld_module.tld("maps.google.com");

console.log(r);