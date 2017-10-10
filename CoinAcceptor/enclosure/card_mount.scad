module rc522_channel() {
    difference() {
        cube([45,8,8]);
        translate([0,3,4]) {
            cube([45,5,1.75]); 
            translate([0,2.5,1.75]) {
                cube([45,5,2.5]); 
            }
        }
        
    }
}

module rc522_holder() {
    rc522_channel();
    translate([0,45.5,0]) {
        mirror([0,1,0]) {
            rc522_channel();
        }
    }
    translate([-15,0,0]) {
        difference() {
            cube([15,45.5,8]);
            rotate([0,-35,0]) {
                cube([15,45.5,8]);   
            }
        }
    }
}

rc522_holder();