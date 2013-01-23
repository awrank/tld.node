var fs = require('fs');
var mysql = require('mysql');
var restler = require('restler');
var punycode = require('punycode');
var module_tld = require('tld.node');
var _ = require("underscore");

var BATCH_SIZE = 100000;
var buffer = [];

var beginTime = startedAt = Date.now();

var totalFetched = 0;
var totalInvalid = 0;
var totalUnknown = 0;
var totalOk = 0;

var zones  = [];
var statsPerZone = {};

function ensureTldReady(callback) {
    if (_.isEmpty(zones)) {
	restler.get("http://data.iana.org/TLD/tlds-alpha-by-domain.txt").on('complete', function(result) {
	    var zl = result.split('\n');
	    
	    _.each(zl, function(z) {
		var domain = z.toLowerCase();
		
		if (domain.indexOf('#') != -1) return;
		
		if (domain.indexOf('xn--') != -1) {
		    try {
		        domain = punycode.toUnicode(domain);
		    } catch(err) {
		        console.log('ERR %s - %s', domain, err);
		    }
		}
		
		if (domain)
		    zones.push(domain);
	    }); 

	    module_tld.load(zones, []); 
	    
	    callback();
	});
    } else callback();
}


var fetchedRows = 0;

var connectionSrc = mysql.createConnection({
    host: 'localhost',
    database: 'crawlerbot',
    user: 'root',
    password : ''
});

connectionSrc.connect();



function validateDomains(list, callback) {
   var recordsIn = list.length;
   var checkStartedAt = Date.now();
	
   var listInvalid = [];
   var listUnknown = [];
   var listOk = [];
	
   ensureTldReady(function(){
	_.each(list, function(v) {
		 var r = module_tld.tld(v.domain);

		 if (r.status == 0) {
			listOk.push(v.domain);
			totalOk++;
			
			var s = statsPerZone[r.domain] || 0;
			statsPerZone[r.domain] = s + 1;
			
		 }

		 if (r.status == 1 || r.status == 2 || r.status == 3) {
			listInvalid.push(v.domain);
			totalInvalid++;
		 }

		 if (r.status == 4) {
			listUnknown.push(v.domain);
			totalUnknown++;
		 }

	});
	

	if (listOk.length > 0) 
		fs.appendFileSync('./domains-ok.txt', listOk.join('\n'), encoding='utf8');

	if (listInvalid.length > 0) 
		fs.appendFileSync('./domains-invalid.txt', listInvalid.join('\n'), encoding='utf8');

	if (listUnknown.length > 0) 
		fs.appendFileSync('./domains-unknown.txt', listUnknown.join('\n'), encoding='utf8');


	console.log('* %s rows ok, %s invalid, %s unknown in %s', listOk.length, listInvalid.length, listUnknown.length, ((Date.now() - checkStartedAt) / 1000));

	callback();
   });
}

function printStats() {
    var keys = [];
    
    for(var key in statsPerZone) {
	keys.push(key);
    }
    
    keys.sort(function(a, b) {
        return statsPerZone[b] - statsPerZone[a];
    });
    
    _.each(keys, function(k) {
	console.log(k + ": " + statsPerZone[k]);
    });
}



	var querySrc = connectionSrc.query(
		'SELECT * FROM domains_processed_20130121'
	);

	querySrc
		.on('error', function(err) {
			// Handle error, an 'end' event will be emitted after this as well
			console.log(err);
		})
		.on('result', function(row) {
			totalFetched++;
			fetchedRows++;

				var domain = row.domain.toLowerCase();
				
				if (domain.charAt(domain.length - 1) == ".")
				    domain = domain.slice(0, -1);
				
				buffer.push({
					idx: row.idx, 
					domain: domain
				});
					
				if (buffer.length >= BATCH_SIZE) {
					console.log('> %s rows fetched (%s) in %s', fetchedRows, totalFetched, ((Date.now() - startedAt) / 1000));
					
					connectionSrc.pause();
					
					validateDomains(buffer, function() {
						connectionSrc.resume();
						fetchedRows = 0;
						buffer = [];
						startedAt = Date.now();
					});
				}
					
		})
		.on('error', function(err) {
			// Handle error, an 'end' event will be emitted after this as well
			console.log(err);
		})
		.on('end', function() {
			// all rows have been received
			startedAt = Date.now();

			if (buffer.length > 0) {
				console.log('> %s rows fetched (%s) in %s', fetchedRows, totalFetched, ((Date.now() - startedAt) / 1000));
					
				validateDomains(buffer, function() {
					console.log('%s rows fetched', totalFetched);
					console.log('%s rows tld invalid', totalInvalid);
					console.log('%s rows tld unknown', totalUnknown);
					console.log('%s rows tld ok', totalOk);
					
					console.log(' in %s sec', ((Date.now() - beginTime) / 1000));
					
					printStats();

					connectionSrc.end();
				});
			} else {
				console.log('%s rows fetched', totalFetched);
				console.log('%s rows tld invalid', totalInvalid);
				console.log('%s rows tld unknown', totalUnknown);
				console.log('%s rows tld ok', totalOk);

				console.log(' in %s sec', ((Date.now() - beginTime) / 1000));
				
				printStats();
				
				connectionSrc.end();
			}
		});















