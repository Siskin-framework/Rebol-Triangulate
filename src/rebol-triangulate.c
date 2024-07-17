#define REAL double

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "triangle.h"
#include "rebol-extension.h"

REBOOL Triangulate(RXIFRM *frm);

RL_LIB *RL; // Link back to reb-lib from embedded extensions

const char *init_block =
	"REBOL [\n"
		"Title: {TRiANGLE}\n"
		"Type: module\n"
		"Name: triangulate\n"
		"Exports: [triangulate]\n"
		"Version: 1.6.0.0\n"
		"License: Apache-2.0\n"
		"Url: https://github.com/Siskin-framework/Rebol-Triangulate\n"
	"]\n"
	"triangulate: command [in [object!] out [object!]]\n"
;

RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	RL = lib;
	//printf("RXinit TRiANGLE %" PRIu64 " %" PRIu64 "\n", sizeof(REBREQ) , sizeof(REBEVT));
	if (!CHECK_STRUCT_ALIGN) {
		printf("failed check");
		return 0;
	}
	return init_block;
}

RXIEXT int RX_Quit(int opts) {
	return 0;
}

RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *ctx) {
	//printf("RX_Call cmd: %i\n", cmd);
	
	switch(cmd) {
		case 0:
			Triangulate(frm);
			break;
	}
	return RXR_VALUE;
}

#define SAFE_FREE(m) if(m) free(m);
#define ASSERT_64BIT_VECT(v) if(VECT_BIT_SIZE(v->size)!=64) {error = "Wrong vector input!"; goto finish;}


/*****************************************************************************/
/*                                                                           */
/*  Do_Report()   Print the input or output.                                 */
/*                                                                           */
/*****************************************************************************/

void Do_Report(
	struct triangulateio *io,
	int markers,
	int reporttriangles,
	int reportneighbors,
	int reportsegments,
	int reportedges,
	int reportnorms)
{
	int i, j;

	//printf("REPORT: %i %i %i %i %i %i\n", markers, reporttriangles, reportneighbors, reportsegments, reportedges, reportnorms);
	if (!io->pointmarkerlist) markers = 0;

	if(io->pointlist) {
		for (i = 0; i < io->numberofpoints; i++) {
			printf("Point %4d:", i);
			for (j = 0; j < 2; j++) {
				printf("  %.6g", io->pointlist[i * 2 + j]);
			}
			if (io->numberofpointattributes > 0) {
				printf("   attributes");
			}
			for (j = 0; j < io->numberofpointattributes; j++) {
				printf("  %.6g",
					io->pointattributelist[i * io->numberofpointattributes + j]);
			}
			if (markers && io->pointmarkerlist != NULL) {
				printf("   marker %d\n", io->pointmarkerlist[i]);
			} else {
				printf("\n");
			}
		}
	}
	printf("\n");

	if ((reporttriangles || reportneighbors) && io->trianglelist && io->numberoftriangleattributes) {
		for (i = 0; i < io->numberoftriangles; i++) {
			if (reporttriangles) {
				printf("Triangle %4d points:", i);
				for (j = 0; j < io->numberofcorners; j++) {
					printf("  %4d", io->trianglelist[i * io->numberofcorners + j]);
				}
				if (io->numberoftriangleattributes > 0) {
					printf("   attributes");
				}
				for (j = 0; j < io->numberoftriangleattributes; j++) {
					printf("  %.6g", io->triangleattributelist[i *
							io->numberoftriangleattributes + j]);
				}
				printf("\n");
			}
			if (reportneighbors && io->neighborlist != NULL) {
				printf("Triangle %4d neighbors:", i);
				for (j = 0; j < 3; j++) {
					printf("  %4d", io->neighborlist[i * 3 + j]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	if (reportsegments && io->segmentlist) {
		for (i = 0; i < io->numberofsegments; i++) {
			printf("Segment %4d points:", i);
			for (j = 0; j < 2; j++) {
				printf("  %4d", io->segmentlist[i * 2 + j]);
			}
			if (markers) {
				printf("   marker %d\n", io->segmentmarkerlist[i]);
			} else {
				printf("\n");
			}
		}
		printf("\n");
	}

	if (reportedges && io->edgelist) {
		for (i = 0; i < io->numberofedges; i++) {
			printf("Edge %4d points:", i);
			for (j = 0; j < 2; j++) {
				printf("  %4d", io->edgelist[i * 2 + j]);
			}
			if (reportnorms && (io->edgelist[i * 2 + 1] == -1)) {
				for (j = 0; j < 2; j++) {
					printf("  %.6g", io->normlist[i * 2 + j]);
				}
			}
			if (markers) {
				printf("   marker %d\n", io->edgemarkerlist[i]);
			} else {
				printf("\n");
			}
		}
		printf("\n");
	}
}


REBOOL Triangulate(RXIFRM *frm) {
	struct triangulateio in, mid, out, vorout;
	REBSER *objIn  = (REBSER*)RXA_OBJECT(frm, 1);
	REBSER *objOut = (REBSER*)RXA_OBJECT(frm, 2);
	RXIARG arg;
	REBSER *vect;
	REBCNT type;
//	REBINT real_bits = sizeof(REAL) << 3;
//	REBINT int_bits  = sizeof(int)  << 3;
	REBOOL report = FALSE;
	char *error = NULL;
	char flags[12];
	
	u32 wReport         = RL_MAP_WORD((REBYTE*)"report");
	u32 wPoints         = RL_MAP_WORD((REBYTE*)"points");
	u32 wAttributes     = RL_MAP_WORD((REBYTE*)"attributes");
	u32 wMarkers        = RL_MAP_WORD((REBYTE*)"markers");
	u32 wRegions        = RL_MAP_WORD((REBYTE*)"regions");
	u32 wSegments       = RL_MAP_WORD((REBYTE*)"segments");
	u32 wSegmentMarkers = RL_MAP_WORD((REBYTE*)"segment-markers");
	u32 wEdges          = RL_MAP_WORD((REBYTE*)"edges");
	u32 wTriangles      = RL_MAP_WORD((REBYTE*)"triangles");
	u32 wVPoints        = RL_MAP_WORD((REBYTE*)"v-points");
	u32 wVEdges         = RL_MAP_WORD((REBYTE*)"v-edges");
	u32 wVNorms         = RL_MAP_WORD((REBYTE*)"v-norms");
	u32 wVAttributes    = RL_MAP_WORD((REBYTE*)"v-attributes");
	
	
	memset(&in,     0, sizeof(triangulateio));
	memset(&mid,    0, sizeof(triangulateio));
	memset(&out,    0, sizeof(triangulateio));
	memset(&vorout, 0, sizeof(triangulateio));

	type = RL_GET_FIELD(RXA_OBJECT(frm, 1), wReport, &arg);
	if(type == RXT_LOGIC && arg.int32a != 0) {
		report = TRUE;
	}

	//puts("points");
	type = RL_GET_FIELD(objIn, wPoints, &arg);
	if(type == RXT_VECTOR) {
		vect = (REBSER*)arg.series;
		ASSERT_64BIT_VECT(vect);
		in.pointlist = (REAL*)vect->data;
		in.numberofpoints = vect->tail >> 1;
		//printf("points: %i %lf %lf,  %lf %lf\n", vect->tail >> 1, in.pointlist[0], in.pointlist[1], in.pointlist[2], in.pointlist[3]);
	}
	else {
		error = "Points must be defined using VECTOR value!";
		goto finish;
	}

	//puts("attributes");
	type = RL_GET_FIELD(objIn, wAttributes, &arg);
	if(type == RXT_VECTOR) {
		vect = (REBSER*)arg.series;
		ASSERT_64BIT_VECT(vect);
		if ((vect->tail % in.numberofpoints) != 0) {
			error = "INVALID NUMBER OF ATTRIBUTES!";
			goto finish;
		}
		in.numberofpointattributes = vect->tail / in.numberofpoints;
		in.pointattributelist = (REAL*)vect->data;
		//printf("attributes: %i %lf %lf %lf\n", vect->tail, in.pointattributelist[0], in.pointattributelist[1], in.pointattributelist[2]);
	}
	else if (type) {
		printf("%u\n", type);
		error = "Attributes must be defined using VECTOR value!";
		goto finish;
	}

	//puts("markers");
	type = RL_GET_FIELD(objIn, wMarkers, &arg);
	if(type == RXT_VECTOR) {
		vect = (REBSER*)arg.series;
		ASSERT_64BIT_VECT(vect);
		if (vect->tail != in.numberofpoints) {
			error = "INVALID NUMBER OF MARKERS (must be same like number of points)!";
			goto finish;
		}
		in.pointmarkerlist = (int *)vect->data;
	}
	else if (type) {
		error = "Markers must be defined using VECTOR value!";
		goto finish;
	}

	//puts("regions");
	type = RL_GET_FIELD(objIn, wRegions, &arg);
	if(type == RXT_VECTOR) {
		vect = (REBSER*)arg.series;
		ASSERT_64BIT_VECT(vect);
		if ((vect->tail % 4) != 0) {
			error = "INVALID NUMBER OF REGIONS (each region is defined using 4 numbers)!";
			goto finish;
		}
		in.numberofregions = vect->tail / 4;
		in.regionlist = (REAL*)vect->data;
	}
	else if (type) {
		error = "Regions must be defined using VECTOR value!";
		goto finish;
	}

	/* Triangulate the points.  Switches are chosen to read and write a  */
	/*   PSLG (p), preserve the convex hull (c), number everything from  */
	/*   zero (z), assign a regional attribute to each element (A), and  */
	/*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
	/*   neighbor list (n).                                              */

	flags[0] = 'p';
	flags[1] = 'c';
	flags[2] = 'z';
	flags[3] = 'e';
	flags[4] = 'v';
	flags[5] = 'Q';
	flags[6] = 'g';
	flags[7] = 0;


	//triangulate("pczAevn", &in, &mid, &vorout);
	//printf("flags %s\n", flags);
	triangulate(flags, &in, &mid, &vorout); // removed: An added Q for no output
	//puts("triangulated");
	if(report) {
		printf("Initial triangulation:\n\n");
		Do_Report(&mid, 1, 1, 1, 1, 1, 0);
		printf("Initial Voronoi diagram:\n\n");
		Do_Report(&vorout, 0, 0, 0, 0, 1, 1);
	}
//	printf("sizeof(REAL): %i real_bits: %i mid.numberofpoints: %i\n", sizeof(REAL), real_bits, mid.numberofpoints);

	if(mid.numberofpoints > 0) {
		type = RL_GET_FIELD(objOut, wPoints, &arg);
		if(type != 0) {
			vect = RL_MAKE_VECTOR(1, 0, 1, 64, 2 * mid.numberofpoints );
			memcpy((void*)vect->data, (const void*)mid.pointlist, sizeof(REAL) * mid.numberofpoints * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wPoints, arg, RXT_VECTOR);
		}

		type = RL_GET_FIELD(objOut, wMarkers, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, mid.numberofpoints);
			memcpy((void*)vect->data, (const void*)mid.pointmarkerlist, sizeof(int) * mid.numberofpoints);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(RXA_OBJECT(frm, 2), wMarkers, arg, RXT_VECTOR);
		}
	}

	if(mid.numberofpointattributes) {
		type = RL_GET_FIELD(objOut, wAttributes, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(1, 0, 1, 64, mid.numberofpointattributes * mid.numberofpoints);
			memcpy((void*)vect->data, (const void*)mid.pointattributelist, sizeof(REAL) * mid.numberofpointattributes * mid.numberofpoints);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wAttributes, arg, RXT_VECTOR);
		}
	}

	if (mid.numberofsegments > 0) {
		type = RL_GET_FIELD(objOut, wSegments, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, 2 * mid.numberofsegments);
			memcpy((void*)vect->data, (const void*)mid.segmentlist, sizeof(int) * mid.numberofsegments * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wSegments, arg, RXT_VECTOR);
		}
		type = RL_GET_FIELD(objOut, wSegmentMarkers, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, mid.numberofsegments);
			memcpy((void*)vect->data, (const void*)mid.segmentmarkerlist, sizeof(int) * mid.numberofsegments);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wSegmentMarkers, arg, RXT_VECTOR);
		}
	}

	if(mid.numberofedges > 0) {
		type = RL_GET_FIELD(objOut, wEdges, &arg);
		if(type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, 2 * mid.numberofedges );
			memcpy((void*)vect->data, (const void*)mid.edgelist, sizeof(int) * mid.numberofedges * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wEdges, arg, RXT_VECTOR);
		}
	}

	if (mid.numberoftriangles > 0) {
		type = RL_GET_FIELD(objOut, wTriangles, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, 3 * mid.numberoftriangles);
			memcpy((void*)vect->data, (const void*)mid.edgelist, sizeof(int) * mid.numberoftriangles * 3);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wTriangles, arg, RXT_VECTOR);
		}
	}

	if (vorout.numberofpoints > 0) {
		type = RL_GET_FIELD(objOut, wVPoints, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(1, 0, 1, 64, 2 * vorout.numberofpoints);
			memcpy((void*)vect->data, (const void*)vorout.pointlist, sizeof(REAL) * vorout.numberofpoints * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wVPoints, arg, RXT_VECTOR);
		}
	}

	if (vorout.numberofedges > 0) {
		type = RL_GET_FIELD(objOut, wVEdges, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(0, 0, 1, 32, 2 * vorout.numberofedges);
			memcpy((void*)vect->data, (const void*)vorout.edgelist, sizeof(int) * vorout.numberofedges * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wVEdges, arg, RXT_VECTOR);
		}
	}
	if (vorout.numberofedges > 0) {
		type = RL_GET_FIELD(objOut, wVNorms, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(1, 0, 1, 64, 2 * vorout.numberofedges);
			memcpy((void*)vect->data, (const void*)vorout.normlist, sizeof(int) * vorout.numberofedges * 2);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wVNorms, arg, RXT_VECTOR);
		}
	}
	if (vorout.numberofpointattributes) {
		type = RL_GET_FIELD(objOut, wVAttributes, &arg);
		if (type != 0) {
			vect = RL_MAKE_VECTOR(1, 0, 1, 64, vorout.numberofpointattributes * vorout.numberofpoints);
			memcpy((void*)vect->data, (const void*)vorout.pointattributelist, sizeof(REAL) * vorout.numberofpointattributes * vorout.numberofpoints);
			arg.series = vect;
			arg.index = 0;
			RL_SET_FIELD(objOut, wVAttributes, arg, RXT_VECTOR);
		}
	}

	/* Attach area constraints to the triangles in preparation for */
	/*   refining the triangulation.                               */

	/* Needed only if -r and -a switches used: */
//	mid.trianglearealist = (REAL *) malloc(mid.numberoftriangles * sizeof(REAL));
//	mid.trianglearealist[0] = 3.0;
//	mid.trianglearealist[1] = 1.0;

	/* Make necessary initializations so that Triangle can return a */
	/*   triangulation in `out'.                                    */

//	out.pointlist = (REAL *) NULL;            /* Not needed if -N switch used. */
//	/* Not needed if -N switch used or number of attributes is zero: */
//	out.pointattributelist = (REAL *) NULL;
//	out.trianglelist = (int *) NULL;          /* Not needed if -E switch used. */
//	/* Not needed if -E switch used or number of triangle attributes is zero: */
//	out.triangleattributelist = (REAL *) NULL;
//
//	/* Refine the triangulation according to the attached */
//	/*   triangle area constraints.                       */
//	printf("================: triangulate('prazBP')\n\n");
//
//	triangulate("prazBP", &mid, &out, (struct triangulateio *) NULL); //removed ra
//
//	printf("Refined triangulation:\n\n");
//	Do_Report(&out, 0, 1, 0, 0, 0, 0); 
	

finish:
	//puts("free..");
	/* Free all allocated arrays, including those allocated by Triangle. */
	//free(in.pointlist);
	//free(in.pointattributelist);
	//free(in.pointmarkerlist);
	//free(in.regionlist);
	SAFE_FREE(mid.pointlist);
	SAFE_FREE(mid.pointattributelist);
	SAFE_FREE(mid.pointmarkerlist);
	SAFE_FREE(mid.trianglelist);
	SAFE_FREE(mid.triangleattributelist);
	SAFE_FREE(mid.trianglearealist);
	SAFE_FREE(mid.neighborlist);
	SAFE_FREE(mid.segmentlist);
	SAFE_FREE(mid.segmentmarkerlist);
	SAFE_FREE(mid.edgelist);
	SAFE_FREE(mid.edgemarkerlist);
	SAFE_FREE(vorout.pointlist);
	SAFE_FREE(vorout.pointattributelist);
	SAFE_FREE(vorout.edgelist);
	SAFE_FREE(vorout.normlist);
	//puts("done");
	if(error == NULL) return TRUE;
	// else on error:
	printf("%s\n", error);
	return FALSE;
}
