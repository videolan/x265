/*****************************************************************************
 * x265: cpu detection logic
 *****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: sumalatha
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/
#include <stdio.h>
#include "vectorclass.h"

#include "cpu_detection.h"


// cpu_detection logic
int cpu_id_detect(void) {
	int cpuid = 0;
    int iset = instrset_detect();                          // Detect supported instruction set
	if(iset < 1)
	{
		//cpuid = 0;
		fprintf(stderr, "\nError: Instruction set is not supported on this computer");
	} else {
		cpuid = iset;
	}
	return cpuid;

}