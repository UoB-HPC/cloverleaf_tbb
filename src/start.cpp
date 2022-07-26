/*
 Crown Copyright 2012 AWE.

 This file is part of CloverLeaf.

 CloverLeaf is free software: you can redistribute it and/or modify it under 
 the terms of the GNU General Public License as published by the 
 Free Software Foundation, either version 3 of the License, or (at your option) 
 any later version.

 CloverLeaf is distributed in the hope that it will be useful, but 
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 details.

 You should have received a copy of the GNU General Public License along with
 CloverLeaf. If not, see http://www.gnu.org/licenses/.
 */


//  @brief Main set up routine
//  @author Wayne Gaudin
//  @details Invokes the mesh decomposer and sets up chunk connectivity. It then
//  allocates the communication buffers and call the chunk initialisation and
//  generation routines. It calls the equation of state to calculate initial
//  pressure before priming the halo cells and writing an initial field summary.


#include "start.h"
#include "build_field.h"
#include "initialise_chunk.h"
#include "generate_chunk.h"
#include "ideal_gas.h"
#include "field_summary.h"
#include "update_halo.h"
#include "visit.h"
#include <string>
#include <sstream>

extern std::ostream g_out;


std::unique_ptr<global_variables> start(parallel_ &parallel,
                                        const global_config &config,
                                        size_t device) {

	if (parallel.boss) {
		g_out << "Setting up initial geometry" << std::endl
		      << std::endl;
	}


	clover_barrier();


	int left, right, bottom, top;
	auto chunkNeighbours = clover_decompose(config, parallel,
	                                        config.grid.x_cells, config.grid.y_cells, left, right,
	                                        bottom, top);

	// Create the chunks

	int x_cells = right - left + 1;
	int y_cells = top - bottom + 1;



    auto globals = std::make_unique<global_variables>(
            config,
            device,
            chunk_type(
                    chunkNeighbours,
                    parallel.task, 1, 1, x_cells, y_cells,
                    left, right, bottom, top,
                    1, config.grid.x_cells,
                    1, config.grid.y_cells,
                    config.tiles_per_chunk)
            );
//	global_variables globals(config,
//	                         omp_device,
//	                         chunk_type(
//			                         chunkNeighbours,
//			                         parallel.task, 1, 1, x_cells, y_cells,
//			                         left, right, bottom, top,
//			                         1, config.grid.x_cells,
//			                         1, config.grid.y_cells,
//			                         config.tiles_per_chunk));


	if (DEBUG) std::cout << "Globals configured" << std::endl;

	auto infos = clover_tile_decompose(*globals, x_cells, y_cells);
	std::transform(infos.begin(), infos.end(), std::back_inserter(globals->chunk.tiles),
	               [](const tile_info &ti) { return tile_type(ti); });



	// Line 92 start.f90
	build_field(*globals);
	if (DEBUG) std::cout << "Field initialised" << std::endl;

	clover_barrier(*globals);

	clover_allocates(*globals, parallel); // FIXME remove; basically no-op, moved to ctor

	if (parallel.boss) {
		g_out << "Generating chunks" << std::endl;
	}

	for (int tile = 0; tile < config.tiles_per_chunk; ++tile) {
		initialise_chunk(tile, *globals);
		generate_chunk(tile, *globals);
	}


	clover_barrier(*globals);
	if (DEBUG) globals->dump("dump_0_after_generate_chunk.txt");

	// Do no profile the start up costs otherwise the total times will not add up
	// at the end
	bool profiler_off = globals->profiler_on;
	globals->profiler_on = false;

	for (int tile = 0; tile < config.tiles_per_chunk; ++tile) {
		ideal_gas(*globals, tile, false);
	}
	if (DEBUG) globals->dump("dump_0_after_ideal_gas.txt");

	// Prime all halo data for the first step
	// TODO replace with std::array
	int fields[NUM_FIELDS];
	for (int &field : fields) field = 0;

	fields[field_density0] = 1;
	fields[field_energy0] = 1;
	fields[field_pressure] = 1;
	fields[field_viscosity] = 1;
	fields[field_density1] = 1;
	fields[field_energy1] = 1;
	fields[field_xvel0] = 1;
	fields[field_yvel0] = 1;
	fields[field_xvel1] = 1;
	fields[field_yvel1] = 1;

	update_halo(*globals, fields, 2);
	if (DEBUG)globals->dump("dump_0_after_update_halo.txt");


	if (parallel.boss) {
		g_out << std::endl
		      << "Problem initialised and generated" << std::endl;
	}

	field_summary(*globals, parallel);

	if (config.visit_frequency != 0) visit(*globals, parallel);

	clover_barrier(*globals);

	globals->profiler_on = profiler_off;

	return (globals);
}

