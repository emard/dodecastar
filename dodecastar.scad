include <polyScrewThread.scad>

/*
todo:
[ ] no thread
[ ] cable strength release
[ ] lamp socket
*/

//create a dodecahedron by intersecting 6 boxes

module pentagonal_cut_up(d, u = 0)
{
  translate([0,0,0.500001-d/2])
    rotate([0,0,90+72/2])
      cylinder(h = d, r1 = 0, r2 = sqrt(2) / 3 - u, $fa = 360/5, $fs = 0.1, center=true);
}

module pentagonal_cut_down(d, u = 0)
{
  translate([0,0,-0.500001+d/2])
    rotate([0,0,90])
      cylinder(h = d, r2 = 0, r1 = sqrt(2) / 3 - u, $fa = 360/5, $fs = 0.1, center = true);
}

/* two pentagonal pyramids, facing peak-to-peak */
/* d - cut depth 
** u - leave uncut edge
*/
module pentagonal_cutter(d, u = 0)
{
  pentagonal_cut_up(d, u);
  pentagonal_cut_down(d, u);
}

/* 
   h = height (mm) (original prism reflector is 43 mm high)

   d = cut depth (unit 0-no cut 1-cut all)
   u = uncut edge roudoff (0-sharp 0.005-slightly rounded)
   l = 0-cut all edges 1-leave top edge uncut

   spiky star with spikes roundoff: d=0.233, u = 0.005
   spiky star with sharp spikes: d=0.233, u = 0

   original:  dodecastar(43, 0.2, u = 0.005);
   improved:  dodecastar(43, 0.233, u = 0.005);
*/
module dodecastar(height = 1, d = 0.233, u = 0.005, l = 0)
{
  scale([height,height,height]) //scale by height parameter
  {
    difference()
    {
      intersection(){
        // make a cube
        cube([2,2,1], center = true); 
        intersection_for(i=[0:4]) //loop i from 0 to 4, and intersect results
        { 
          //make a cube, rotate it 116.565 degrees (dihedral angle)  around the X axis,
          // dihedral angle:
          // golden_ratio = (1 + sqrt(5)) / 2
          // dihedral_angle = 2 * atan(golden_ratio)
          // dihedral_angle = 2 * atan((1 + sqrt(5)) / 2)
          //then 72*i around the Z axis
          rotate([0,0,72*i])
            rotate([2 * atan((1 + sqrt(5)) / 2),0,0])
              cube([2,2,1], center = true); 
        }
      }

      union()
      {
        rotate([0,0,72/2])
          if(l < 0.5)
            pentagonal_cutter(d, u);
          else
            pentagonal_cut_down(d, u);
        for(i = [0:4])
        {
          rotate([0,0,72*i])
            rotate([2 * atan((1 + sqrt(5)) / 2),0,0])
              pentagonal_cutter(d, u);
        }
      }
    }
  }
}

//create a dodecahedron by intersecting 6 boxes
module dodecahedron(height) 
{
        scale([height,height,height]) //scale by height parameter
        {
                intersection(){
                        //make a cube
                        cube([2,2,1], center = true); 
                        intersection_for(i=[0:4]) //loop i from 0 to 4, and intersect results
                        { 
                                //make a cube, rotate it dihedral angle 116.565 degrees around the X axis,
                                // dihedral angle:
                                // golden_ratio = (1 + sqrt(5)) / 2
                                // dihedral_angle = 2 * atan(golden_ratio)
                                // dihedral_angle = 2 * atan((1 + sqrt(5)) / 2)
                                //then 72*i around the Z axis
                                rotate([0,0,72*i])
                                        rotate([2 * atan((1 + sqrt(5)) / 2),0,0])
                                        cube([2,2,1], center = true); 
                        }
                }
        }
}

/* make a lamp hole in the reflector 
** height in mm
** ih: interior dodecastar lamp hole factor (0.7) relative to height
** id: interior dodecastar cut depth (0.15)
** extup: screw hole reinforcement height factor (0.25) relative to height
** interface 0-none, 1-screw hole, 2-ledstrip pass-thru, 3-ledstrip half with 2 screws
interior dodecastar : ih = 0.7, id = 0.15, extup = 0.25
interior simple dodecahedron : ih = 0.5, id = 0, extup = 0.25
*/
module reflector(height = 43, ih = 0.5, id = 0.0, extup = 0.22,
// for interface=3
screw_pos=7,
screw=2.2, screw_thread_d=2.2*0.8, screw_hole_d=2.2*1.3, screw_head_d=2.2*2.2,
screw_head_h=2.2*0.6, screw_length=9.5,
nut_height=1.0, nut_d=2.2*2.1,
cable_w=6, cable_h=1.8,
interface=3)
{
  // additional stuff for interface=3
  star_angle=atan((1 + sqrt(5)) / 2);
  screw_height=screw_length/2-screw_head_h; // height without the head
  rotate([0,interface==3 ? 270 : 0,0])
  union()
  {
    difference()
    {
      union() // material volume
      {
        dodecastar(height);
        /* lamp screw hole, reinforced with extruded pentagon
        ** 0.233 is the cut depth of outer dodecastar
        ** 0.1 is the internal dodecastar-hole cut depth */
        if(interface == 1)
          translate([0,0,height*ih/2 * (1-2*id)])
            rotate([0,0,90])
              cylinder(h = height*(extup+1-0.233*2-ih*(1-2*id))/2, r = 8, $fa = 360/5, $fs = 0.1);
      }
      union() // void volume
      {
        /* cut out dodecastar interior for the lamp */
        // dodecahedron(height*ih);
        dodecastar(height*ih, d = id, u = 0, l = 0);
        /* cut out the screw thread, translate and length same as reinforcement */
/*           8,  // Height 
 *           3,  // Step height (the half will be used to countersink the ends)
 *          55,  // Degrees (same as used for the screw_thread example)
 *          15,  // Outer diameter of the thread to match
 *         0.5)  // Resolution, you may want to set this to small values cca 0.5
 *                  (quite high res) to minimize overhang issues 
 */
        if(interface==1)
          translate([0,0,height*ih/2 * (1-2*id)])
            thread_cut(height*(extup+1-0.233*2-ih*(1-2*id))/2,3,55,12.4,0.5);
        if(interface==2)
          rotate([0,0,30])
          {
            translate([0,-2,0])
              cube([9,3,height], center=true);
            cube([13,1,height], center=true);
          }
        if(interface==3)
        {
          // cube to cut off half-star
          translate([-height,0,0])
            cube([2*height,2*height,2*height], center=true);
          // box to cut off cable holder slit
          rotate([star_angle,0,0])
            cube([cable_h,cable_w,height*1.1],center=true);
          // cylinder to drill screw thru-hole
          // just drill screw head hole thru above
          rotate([0,90,0])
            rotate([0,0,star_angle])
              translate([0,screw_pos,screw_height])
                cylinder(d=screw_head_d-0.001,h=height+0.002,$fn=20,center=false);
          // screw leader hole thru all
          rotate([0,90,0])
            rotate([0,0,star_angle])
              translate([0,screw_pos,-0.001])
                cylinder(d=screw_hole_d,h=height+0.002,$fn=20,center=false);
          // screw hole for the thread
          rotate([0,90,0])
            rotate([0,0,star_angle])
              translate([0,-screw_pos,-0.001])
                cylinder(d=screw_thread_d,h=height*0.22+0.002,$fn=20,center=false);
          // screw leader hole for thread clearance
          rotate([0,90,0])
            rotate([0,0,star_angle])
              translate([0,-screw_pos,-0.001])
                cylinder(d=screw_hole_d,h=nut_height+0.002,$fn=20,center=false);
        }
      }
    }
    if(interface == 3) // post-added volume
    {
      // plastic nut for screw thread
      rotate([0,90,0])
        rotate([0,0,star_angle])
        union()
        {
           translate([0,-screw_pos,height/10+nut_height/2])
             difference()
             {
               cylinder(d=nut_d,h=height/5-nut_height,$fn=20,center=true);
               // hole in the nut for screw thread
               cylinder(d=screw_thread_d,h=height,$fn=20,center=true);
             }
           translate([0,screw_pos,0])
             difference()
             {
               union()
               {
               // the material (the nut)
               // 1.5 mm add to diameter for screw holder thickness
               translate([0,0,nut_height])
                 cylinder(d1=nut_d,d2=screw_head_d+1.5,h=screw_height-nut_height,$fn=20,center=false);
               translate([0,0,screw_height])
                 cylinder(d=screw_head_d+1.5,h=height*0.22-screw_height,$fn=20,center=false);
               }
               // hole in the screw leader
               union()
               {
                 // thru-hole, more than long enough
                 cylinder(d=screw_hole_d, h=height, $fn=20, center=true);
                 // conical screw head
                 translate([0,0,screw_height])
                   cylinder(d1=screw_hole_d,d2=screw_head_d,h=screw_head_h, $fn=20, center=false);
                 translate([0,0,screw_height+screw_head_h])
                   cylinder(d=screw_head_d,h=height,$fn=20,center=false);
               }
             }
        }
    }
  }
}

/* for 3d printing */
/* minimal size (cca 0.5x normal size) that fits ledstrip */
if(1)
    reflector(height=27, screw_head_h=0.001, screw_length=5, cable_h=2.2, interface=3);

/* normal size */
if(0)
    reflector(height=43, screw_pos=43/3,screw_head_h=0.001,screw_length=5,interface=3);

/* 2x the normal size */
if(0)
    reflector(height=86, screw_pos=86/3,screw_head_h=0.001,screw_length=5,interface=3);

/* 2x the normal (big star) vertical */
if(0)
    reflector(height=86, screw_pos=86/3,screw_head_h=0.001,screw_length=5,interface=0);
/* holder for the star of 2x normal size */
if(0)
{
    h_hold_star=0.1; // top straight height holding the star
    h_holder=81; // narrowing height
    n_hold=5; // number of angles
    thick_holder=2;
    d2_holder=50; // inner small diameter
    d1_holder=68; // inner big diameter
    translate([0,0,70])
    rotate([0,0,-18])
    difference()
    {
      union()
      {
        translate([0,0,-h_holder/2-h_hold_star/2])
          cylinder(d=d1_holder+2*thick_holder,h=h_hold_star,$fn=n_hold,center=true);
        cylinder(d1=d1_holder+2*thick_holder, d2=d2_holder+2*thick_holder,h=h_holder,$fn=n_hold,center=true);
      }
      translate([0,0,-h_holder/2-h_hold_star/2])
          cylinder(d=d1_holder,h=h_hold_star+0.1,$fn=n_hold,center=true);
      cylinder(d1=d1_holder, d2=d2_holder,h=h_holder+0.1,$fn=n_hold,center=true);
      for(j=[-3:2])
      for(i=[0:n_hold-1])
          translate([0,0,j*15])
          rotate([0,0,(i+j/4)*360/n_hold])
            rotate([0,90,0])
              cylinder(d=14-j,h=100,$fn=6,center=true);

    }
}

/* for blender export */
/*
scale([5/43,5/43,5/43])
//  reflector(43, ih=0.8, id=0.20, extup = 0.25);
 reflector(43, ih=0.5, id=0.0, extup = 0.25);
// dodecastar(43, 0.233, 0.005, 1);
*/

/*
dodecahedron(43);
scale(43,43,43)
  translate([0,0,0.5 + 0.1/2])
    rotate([0,0,90])
      cylinder(h = 0.1, r = sqrt(2)/3, $fa = 360/5, $fs = 0.1, center=true); // cut
      cylinder(h = 0.1, r = (1 + sqrt(5)) / sqrt(2) / 6 / cos(36), $fa = 360/5, $fs = 0.1, center=true); // inscribed
*/

/* 
 * A screw, threaded all the way, with hex head.
 *
 * hex_screw(12,  // Outer diameter of the thread
 *            3,  // Thread step
 *           55,  // Step shape degrees
 *            9,  // Length of the threaded section of the screw
 *          1.5,  // Resolution (face at each 2mm of the perimeter)
 *            1,  // Countersink on top ends
 *           13,  // Distance between flats for the hex head
 *            2,  // Height of the hex head (can be zero)
 *            0,  // Length of the non threaded section of the screw
 *            0)  // Diameter for the non threaded section of the screw
 *                     -1 - Same as inner diameter of the thread
 *                      0 - Same as outer diameter of the thread
 *                  value - The given value
 *
 */

if(0)
translate([0,0,38.2])
  rotate([180,0.0])
    hex_screw(12,3,55,6,1.5,1,13,2,0,0);

