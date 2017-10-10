wall = 6; // thickness of material for enclosure
depth = 10; // LCD protrusion from PCB (metal casing)

difference() {
    union() {
        // back panel (glue or epoxy to enclosure material)
        translate([-20,-20,0]) {
            cube([133, 95, depth - wall]);
        }

        // panel hole
        color("green") {
            translate([0,0,depth - wall -1]) {
                minkowski() {
                    // 108 x 70 with 7.5 corner radius
                    cube([93, 55, wall]);
                    cylinder(r=7.5,h=1, $fn=32);
                }   
            }
        }
    }

    union() {
        // module cutout (metal casing)
        translate([-3.5,6.5,0]){
            color("red")
            cube([100, 42, depth-1]);
        }

        // screen cutout
        translate([8.5,14.5,0]){
            cube([76, 26, depth]);
        }

        // pins
        translate([7.5 ,55 - 5 + 1.5,0]){
            cube([45, 5, depth- 3]);
        }

        // mounting holes
        cylinder(d=3, h=depth - 3, $fn = 32);
        translate([93,0,0]){
            cylinder(d=3, h=depth - 3, $fn = 32);
        }
        translate([93,55,0]){
            cylinder(d=3, h=depth - 3, $fn = 32);
        }
        translate([0,55,0]){
            cylinder(d=3, h=depth - 3, $fn = 32);
        }
    }
}