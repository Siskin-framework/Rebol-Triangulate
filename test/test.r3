Rebol [
   title: "Triangulate extension test"
]

random/seed 2021

unless value? 'triangulate [
   target: rejoin [system/build/os #"-" system/build/arch]
   print as-yellow "Trying to import Triangulate extension..."
   try/except [
      import to-real-file rejoin [%../triangulate- target %.rebx]
   ][
      print as-purple system/state/last-error
      print as-red "Import failed!"
      halt
   ]
]

inp: object [
   points: #[f64! [
       0.0   0.0
       10.0  0.0
       10.0  10.0
       0.0   10.0
       2.5   2.5
   ]]
   attributes: #[f64! [
       100.0
       2.0
       3.0
       4.0
       5.0
   ]]
   markers: #[i64! [2 1 1 1 1]]
   regions: #[f64! [
      5.0
      5.0
      10.0 ; Regional attribute (for whole mesh).
      1.0  ; Area constraint that will not be used.
   ]]
   report: true
]

out: object [
	points:          none
	attributes:      none
	markers:         none
	segments:        none
	segment-markers: none
	edges:           none
	triangles:       none

	; voronoi output:
	v-points:        none
	v-attributes:    none
	v-edges:         none
	v-norms:         none  
]
? inp
triangulate inp out
? out