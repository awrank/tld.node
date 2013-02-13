var console = require('console'),
    vows = require('vows'),
    assert = require('assert'),
    fs = require('fs');


var suite = vows.describe('Testing dynamic TLD database updates');


/**
 * Test suite for main functionality of TLD module.
 *
 * There is list of test scenarios (13):
 *
 * - when passing unknown top level domain (scarlet.nl)
 * - when dynamically adding top level domain (nl)
 * - when passing active dynamically added top level domain (www.scarlet.nl)
 * - when dynamically removing top level domain (com.ua)
 * - when passing removed top level domain with two words zone (www.pravda.com.ua), still should be resolved by top level zone (ua)
 * - when dynamically removing top level domain (net.ua)
 * - when passing removed top level domain with two words zone (www.deshevle.net.ua), still should be resolved by top level zone (ua)
 * - when dynamically removing top level domain (ua)
 * - when passing removed top level domain (rozetka.ua)
 * - when passing removed top level domain with two words zone (www.pravda.com.ua) after top level zone (ua) removed
 * - when passing removed top level domain with two words zone (www.deshevle.net.ua) after top level zone (ua) removed
 * - when dynamically adding domain to list of reserved domains (example.net)
 * - when passing dynamically reserved domain (example.net)
 */
suite.addBatch({
    'tld-module': {
        topic: function() { 
        	var tld_module = require("../build/Release/module_tld.node"); 

        	var active_tld = ['com','net','ua','com.ua','net.ua','ru','*.ar','!uba.ar','рф'];
        	var reserved_tld = ['example.com'];
        	
        	tld_module.load(active_tld, reserved_tld);
        	
        	return tld_module;
        },
		
		'when passing unknown top level domain (scarlet.nl)': {
		    topic: function(tld_module) {
				return tld_module.tld('scarlet.nl');
		    },
		    'domain should be rejected with status "NOTFOUND"': function (result) {
		    	assert.equal (result.status, 4);
		    }
		},
		
		'when dynamically adding top level domain (nl)': {
		    topic: function(tld_module) {
				return tld_module.update('nl', 1 /* to add domain */, 1 /* to tld-base */);
		    },
		    'empty object has to be returned': function (result) {
		    	assert.ok(result);
		    }
		},
		
		'when passing active dynamically added top level domain (www.scarlet.nl)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.scarlet.nl');
		    },
		    'domain should be validated with status "SUCCESS"': function (result) {
		    	assert.equal (result.status, 0);
		    },
		    'top level domain should be "nl"': function (result) {
		    	assert.equal (result.domain, 'nl');
		    },		    
		    'has to have two sub domains': function (result) {
		    	assert.equal (result.sub_domains.length, 2);
		    },
			'second sub domain should be "scarlet"': function (result) {
				assert.equal (result.sub_domains[1], 'scarlet');
			},
		    'first sub domain should be "www"': function (result) {
		    	assert.equal (result.sub_domains[0], 'www');
		    }
		}, 
						
		'when dynamically removing top level domain (com.ua)': {
		    topic: function(tld_module) {
				return tld_module.update('com.ua', 2 /* to remove domain */, 1 /* from tld-base */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing removed top level domain with two words zone (www.pravda.com.ua), still should be resolved by top level zone (ua)': {
		    topic: function(tld_module) {
				return tld_module.tld('www.pravda.com.ua');
		    },
            'domain should be validated with status "SUCCESS"': function (result) {
                assert.equal (result.status, 0);
            },
            'top level domain should be "ua"': function (result) {
                assert.equal (result.domain, 'ua');
            },
            'has to have three domains': function (result) {
                assert.equal (result.sub_domains.length, 3);
            }
		},

        'when dynamically removing top level domain (net.ua)': {
            topic: function(tld_module) {
                return tld_module.update('net.ua', 2 /* to remove domain */, 1 /* from tld-base */);
            },
            'empty object has to be returned': function (result) {
                assert.ok(result);
            }
        },

        'when passing removed top level domain with two words zone (www.deshevle.net.ua), still should be resolved by top level zone (ua)': {
            topic: function(tld_module) {
                return tld_module.tld('www.deshevle.net.ua');
            },
            'domain should be validated with status "SUCCESS"': function (result) {
                assert.equal (result.status, 0);
            },
            'top level domain should be "ua"': function (result) {
                assert.equal (result.domain, 'ua');
            },
            'has to have three domains': function (result) {
                assert.equal (result.sub_domains.length, 3);
            }
        },

		'when dynamically removing top level domain (ua)': {
		    topic: function(tld_module) {
				return tld_module.update('ua', 2 /* to remove domain */, 1 /* from tld-base */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing removed top level domain (rozetka.ua)': {
		    topic: function(tld_module) {
				return tld_module.tld('rozetka.ua');
		    },
			'domain should be rejected with status "NOT FOUND"': function (result) {
				assert.equal (result.status, 4);
			}
		},

        'when passing removed top level domain with two words zone (www.pravda.com.ua) after top level zone (ua) removed': {
            topic: function(tld_module) {
                return tld_module.tld('www.pravda.com.ua');
            },
            'domain should be rejected with status "NOT FOUND"': function (result) {
                assert.equal (result.status, 4);
            }
        },

        'when passing removed top level domain with two words zone (www.deshevle.net.ua) after top level zone (ua) removed': {
            topic: function(tld_module) {
                return tld_module.tld('www.deshevle.net.ua');
            },
            'domain should be rejected with status "NOT FOUND"': function (result) {
                assert.equal (result.status, 4);
            }
        },
		
		'when dynamically adding domain to list of reserved domains (example.net)': {
		    topic: function(tld_module) {
				return tld_module.update('example.net', 1 /* to add domain */, 2 /* from reserved-base */);
		    },
			'empty object has to be returned': function (result) {
				assert.ok(result);
			}
		},
		
		'when passing dynamically reserved domain (example.net)': {
		    topic: function(tld_module) {
				return tld_module.tld('example.net');
		    },
			'domain should be rejected with status "RESERVED"': function (result) {
				assert.equal (result.status, 1);
			}
		}
    }
});


suite.export(module);
