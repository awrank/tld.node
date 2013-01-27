var
    fs = require('fs'),
    tld_module = require('tld.node');

// load top level domains from file base.dat
var active_tld = fs.readFileSync("base.dat");

// load reserved domains from file guide.dat
var reserved_tld = fs.readFileSync("guide.dat");

// initialize module
tld_module.load(active_tld, reserved_tld);

// check domain
var result = tld_module.tld("maps.google.com");

console.log(result);