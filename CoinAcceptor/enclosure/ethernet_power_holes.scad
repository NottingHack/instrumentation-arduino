difference() {
    union() {
        cube([43, 32, 6]);
        translate([-5,-5, 6]) {
            // internal dimension requirements
            cube([53, 42, 4]);
        }
    }
    translate([5,5,0]) {
        cube([16,22,10]);
    }
    
    translate([32,14.5 + 6,0]) {
        cylinder(d=12, h=10, $fn=32);
        translate([0,0,5]) {
            cylinder(d=18, h=10, $fn=32);
        }
    }
}


translate([5,5,0]) {
    
    difference() {
        
        cube([16,22,2]);

        translate([2,10,0]) {
            difference() {
                    
                color("blue") {
                    cube([12,11,2]);
                }
                
                union() {
                    translate([0,7,0]) {
                        cube([2.5,4,2]);
                    }
                    
                    translate([9.5,7,0]) {
                        cube([2.5,4,2]);
                    }    
                    
                    translate([2.5,9,0]) {
                        cube([1.25,2,2]);
                    }    
                    
                    translate([8.25,9,0]) {
                        cube([1.25,2,2]);
                    }    
                }
            }
        }
    }
}