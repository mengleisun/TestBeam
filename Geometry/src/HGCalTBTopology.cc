#include "HGCal/Geometry/interface/HGCalTBTopology.h"
#include "HGCal/Geometry/interface/HGCalTBCellParameters.h"
#include "math.h"
#include <stdlib.h>
#define PI 3.14159265

bool HGCalTBTopology::iu_iv_valid(int layer, int sensor_iu, int sensor_iv, int iu, int iv, int sensorSize) const
{
	int aiv = abs(iv);
	int iuc = (iv < 0) ? (-iu) : (iu);
	if(layer <= 28 && sensor_iu == 0 && sensor_iv == 0) {
		if(sensorSize == 128) {
			if (iv == 0) return (iu >= -5 && iu <= 5);
			else if (aiv == 1) return (iuc >= -6 && iuc <= 5);
			else if (aiv == 2) return (iuc >= -6 && iuc <= 4);
			else if (aiv == 3) return (iuc >= -7 && iuc <= 4);
			else if (aiv == 4) return (iuc >= -7 && iuc <= 3);
			else if (aiv == 5) return (iuc >= -6 && iuc <= 1);
			else if (aiv == 6) return (iuc >= -5 && iuc <= -1);
			else if (aiv == 7) return (iuc == -3 || iuc == -4);
			else return false;
		} else return false;
	} else return false;
}

double HGCalTBTopology::Cell_Area(int cell_type) const
{
	double a = HGCAL_TB_CELL::FULL_CELL_SIDE;
	double b = HGCAL_TB_CELL::CALIB_PAD_SIDE;
	double mouse_bite = HGCAL_TB_CELL::MOUSE_BITE_SIDE;

	double area_full_hex = 0.5 * 3 * sqrt(3) * pow(a, 2);
	double area_calib_pad = 0.5 * 3 * sqrt(3) * pow(b, 2);
	double area_half_hex = area_full_hex / 2;
	double area_mouse_bite = area_half_hex - (0.5 * pow((a - mouse_bite), 2) / tan(PI / 6));
	double area_outer_calib_pad = area_full_hex - area_calib_pad;
	double area_merged_cell = area_full_hex + area_half_hex;

	if (cell_type == 0) return area_full_hex;
	else if (cell_type == 1) return area_calib_pad;
	else if (cell_type == 2) return area_half_hex;
	else if (cell_type == 3) return area_mouse_bite;
	else if (cell_type == 4) return area_outer_calib_pad;
	else if (cell_type == 5) return area_merged_cell;
	else return -1.; //signifies an invalid cell type
}

