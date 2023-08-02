#include <cstddef>
#include <memory>
#include <cstdlib>

#define next_float() (float) (rand()) / (float) RAND_MAX


float *generate_worley_cells_3d(std::size_t num_cells)
{
	float cell_size = 1.0f / num_cells;

	std::size_t len = num_cells * num_cells * num_cells * 3;
	float *buf = new float[len];

	if (! buf)
		return buf;

	std::size_t x = 0;
	std::size_t y = 0;
	std::size_t z = 0;

	for (int i = 0; i < len; i += 3)
	{
		float px = (x + next_float()) * cell_size;
		float py = (y + next_float()) * cell_size;
		float pz = (z + next_float()) * cell_size;

		buf[i] = px;
		buf[i + 1] = py;
		buf[i + 2] = pz;

		// Go to next cell
		++z;
		if (z >= num_cells)
		{
			++y;
			if (y >= num_cells)
				++x;
			y %= num_cells;
		}
		z %= num_cells;
	}

	return buf;
}
