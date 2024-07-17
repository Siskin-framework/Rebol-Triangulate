Rebol [
	Title: "Trianguale image (resize and add line effect)"
	needs: 3.11.0 ; to automatically install Blend2D extension
]

random/seed 2021

CI?: any [
	"true" = get-env "CI"
	"true" = get-env "GITHUB_ACTIONS"
	"true" = get-env "TRAVIS"
	"true" = get-env "CIRCLECI"
	"true" = get-env "GITLAB_CI"
]

if CI? [
	;; make sure that we load a fresh extension
	try [system/modules/triangulate: none]
	;; use project's root directory as a modules location
	system/options/modules: dirize to-real-file %../
]

import 'triangulate
import 'blend2d ;- Blend2D is used to draw images


inp-img: 
out-img: none

sc:          10.0
pwr:         6.9
particles:   24000
light-mode?: on

bg:    0.0.0   
bg2:   0.0.0.10 ; used to fade image between steps
ln-w1: 1
ln-w2: 4
ln-c1: 0.28.0.128
ln-c2: 0.155.55.200
pt-w:  16

mode:  3
steps: 50

inp: object [
	points:     make vector! compose [decimal! 64 (particles * 2)]
	attributes: make vector! compose [decimal! 64 (particles)]
;	report:     true
]
out: object [
	points:     none
	attributes: none
	edges:      none
	triangles:  none

	; voronoi output:
	v-points:  none
	v-edges:   none
	v-norms:   none
]

drw-points: make block! 4 * particles

init-points: func [
	particles [integer!]
	image     [image!]
	pwr       [number!]
	/local size p c
][
	insert clear drw-points [
		line-width 0 point-size :pt-w alpha 30%
	]
	size: image/size
	while [particles > 0][
		p: random size
		rgb: image/(p) 
		hsv: rgb-to-hsv rgb
		accepted?: either light-mode? [
			(random 0.5) < power (hsv/:mode / 255) pwr
		][	(random 0.5) < power ((255 - hsv/:mode) / 255) pwr]

		if accepted? [
			inp/points/1: p/x: (p/x * sc) - random sc
			inp/points/2: p/y: (p/y * sc) - random sc
			inp/points: skip inp/points 2
			inp/attributes/:particles: to-integer to-binary rgb
			repend drw-points ['fill rgb 'point p] 
			-- particles
		]
	]
	inp/points: head inp/points
	;? inp
	inp
]

draw-frame: does [
	;prin "init points: " print dt [
		init-points particles inp-img pwr
	;]
	;prin "triangulate: " print dt [
		triangulate inp out
	;]

	;if find out 'v-points [sc * out/v-points]

	if block? select out 'v-edges [
		temp:  make block! (length? out/v-points) / 2
		norms: make block! 1000
		while [not tail? out/v-edges][
			either out/v-edges/2 = -1 [
				i: index? out/v-edges
				norm-x: pick out/v-norms i
				norm-y: pick out/v-norms i + 1
				;print [norm-x norm-y]
				append append norms out/v-edges/1 out/v-edges/2
				append append norms norm-x norm-y
			][
				append append temp out/v-edges/1 out/v-edges/2
			]
			out/v-edges: skip out/v-edges 2	
		]
		;? norms
		out/v-edges: make vector! compose/only [integer! 32 (temp)]
	]
	;? out/v-points
	;prin "draw image:  "
	;print dt [
		draw out-img [
			fill :bg2 fill-all fill off
			point-size 4
		;	fill white  point :out/points
			fill 55.200.100.100 point :out/v-points
			line-width :ln-w1 pen :ln-c1
			line :out/points :out/edges
			line-width :ln-w2 pen :ln-c2
			line :out/v-points :out/v-edges
		]
		draw out-img drw-points
	;]
	;? out/v-norms
	;? out
	;print "draw done"
]



triangulate-image: func[file [file!]][
	print [as-green "Triangulating image:" as-yellow file]
	inp-img: load file
	;out-img: resize inp-img sc * inp-img/size ;
	out-img: make image! reduce [sc * inp-img/size bg]
	out-file: join %out/ file

	print dt [loop steps [draw-frame]]

	save out-file resize out-img 50%
	;view load out-file
	out-file
]

make-dir %out/

ln-c1: 0.28.0.128
ln-c2: 0.55.55.200
ln-w2: 8
steps: 1
triangulate-image %Vermeer.jpg

ln-c1: 255.0.0.130
ln-c2: 0.55.55.200
ln-w2: 6
steps: 10
particles: 14000
triangulate-image %Spiral_Tribe.jpg


halt
