#include <endian.h>
#include <stdio.h>
#include <string.h>

#define FILE_HEADER 8

// Variable-length encode the offset using high two bits
// of first byte, depending on how many bytes the offset
// requires.
//
// Decoding left shifts by 2 to get back original value
int out(FILE *p, int cursor, int diff)
{
	static int count = 0;
	int sp = cursor - diff;
	if (sp <= 0xFC) {
		sp = (sp >> 2) | 0x40;
		fwrite(&sp, sizeof(char), 1, p);
		count++;
	} else if (sp <= 0xFFFC) {
		sp = (sp >> 2) | 0x8000;
		sp = htobe16(sp);
		fwrite(&sp, sizeof(char), 2, p);
		count += 2;
	} else {
		sp = (sp >> 2) | 0xC0000000;
		sp = htobe32(sp);
		fwrite(&sp, sizeof(char), 4, p);
		count += 4;
	}
	return count;
}

void generate_pof0(FILE *f, FILE *p)
{
	int byte_count = 0;
	int pof0_offset;
	int temp, temp1;
	int mesh_count, bone_count, tex_count, obj_group_count;;
	int mesh_offset, bone_offset, texname_offset, obj_groupname_offset;
	unsigned char word[4] = "POF0";
	int cursor;

	// Write POF0 header
	fwrite(word, sizeof(char), 4, p);

	// Write the size, to be overwritten at the end
	fwrite(&byte_count, sizeof(int), 1, p);

	// Read file header and extract top level offsets
	fread(&word, 1, sizeof(int), f);
	if (strncmp(word, "JBOY", 4) != 0) {
		printf("Invalid YOBJ file, JBOY header messing\n");
		return;
	}
	fread(&pof0_offset, 1, sizeof(int), f);
	pof0_offset = htobe32(pof0_offset);
	//printf("pof0 offset: %.8X\n", pof0_offset);
	fread(&temp, 1, sizeof(int), f); // zero
	fread(&temp, 1, sizeof(int), f); // pof0 offset again
	if (htobe32(temp) != pof0_offset) {
		printf("Invalid YOBJ file, second pof0 offset different from first\n");
		return;
	}
	fread(&temp, 1, sizeof(int), f); // zero
	fread(&temp, 1, sizeof(int), f); // zero
	fread(&mesh_count, 1, sizeof(int), f);
	mesh_count = htobe32(mesh_count);

	cursor = ftell(f);
	out(p, cursor, FILE_HEADER);
	fread(&mesh_offset, 1, sizeof(int), f);
	mesh_offset = htobe32(mesh_offset);

	fread(&bone_count, 1, sizeof(int), f);
	bone_count = htobe32(bone_count);
	fread(&tex_count, 1, sizeof(int), f);
	tex_count = htobe32(tex_count);

	temp = cursor;
	cursor = ftell(f);
	out(p, cursor, temp);
	fread(&bone_offset, 1, sizeof(int), f);
	bone_offset = htobe32(bone_offset);

	temp = cursor;
	cursor = ftell(f);
	out(p, cursor, temp);
	fread(&texname_offset, 1, sizeof(int), f);
	texname_offset = htobe32(texname_offset);

	temp = cursor;
	cursor = ftell(f);
	out(p, cursor, temp);
	fread(&obj_groupname_offset, 1, sizeof(int), f);
	obj_groupname_offset = htobe32(obj_groupname_offset);

	fread(&obj_group_count, 1, sizeof(int), f);
	obj_group_count = htobe32(obj_group_count);

	fread(&temp, 1, sizeof(int), f); // zero
	fread(&temp, 1, sizeof(int), f); // zero

	// Iterate meshes extracting per-mesh offsets	
	for (int i = 0; i < mesh_count; i++) {
		// start of mesh data
		fseek(f, mesh_offset + FILE_HEADER + 180 * i, SEEK_SET);
		// jump to relevant offsets for the given mesh
		fseek(f, 104, SEEK_CUR);

		temp = cursor;
		cursor = ftell(f);
		out(p, cursor, temp);
		fread(&temp1, 1, sizeof(int), f); // vertex offsets

		temp = cursor;
		cursor = ftell(f);
		out(p, cursor, temp);
		fread(&temp1, 1, sizeof(int), f); // weight offsets

		temp = cursor;
		cursor = ftell(f);
		out(p, cursor, temp);
		fread(&temp1, 1, sizeof(int), f); // UV offsets

		// jump over to the next relevant section 
		fseek(f, 32, SEEK_CUR);

		temp = cursor;
		cursor = ftell(f);
		out(p, cursor, temp);
		fread(&temp1, 1, sizeof(int), f); // tex offsets

		temp = cursor;
		cursor = ftell(f);
		out(p, cursor, temp);
		fread(&temp1, 1, sizeof(int), f); // draw offsets
	}
	// Iterate meshes extracting per-mesh info.
	// Separate loop to keep the offsets in ascending order.
	for (int i = 0; i < mesh_count; i++) {
		int draw_counts, tex_counts;
		// start of mesh data
		// start of mesh data
		fseek(f, mesh_offset + FILE_HEADER + 180 * i, SEEK_SET);
		fread(&temp1, 1, sizeof(int), f); // vertex counts
		temp1 = htobe32(temp1);
		//printf("vertex counts: %.2X\n", temp1);
		fread(&draw_counts, 1, sizeof(int), f); // draw counts
		draw_counts = htobe32(draw_counts);
		//printf("draw counts: %.2X\n", draw_counts);

		// jump over to the next relevant section 
		fseek(f, 136, SEEK_CUR);
		fread(&tex_counts, 1, sizeof(int), f); // tex counts
		tex_counts = htobe32(tex_counts);

		temp = cursor;
		fread(&temp1, 1, sizeof(int), f); // tex offsets
		cursor = htobe32(temp1) + 8;
		for (int j = 0; j < tex_counts; j++) {
			out(p, cursor + 4*j, temp);
			temp = cursor + 4*j;
		}

		// jump back
		fseek(f, -48, SEEK_CUR);

		//temp = cursor;
		fread(&temp1, 1, sizeof(int), f); // vertex offsets
		cursor = htobe32(temp1) + 8;
		out(p, cursor, temp);

		// jump forward again
		fseek(f, 44, SEEK_CUR);

		temp = cursor;
		fread(&temp1, 1, sizeof(int), f); // draw offsets
		cursor = htobe32(temp1) + 16;
		out(p, cursor, temp);

		int draw_offsets = cursor;
		for (int j = 1; j < draw_counts; j++) {
			temp = cursor;
			cursor = draw_offsets + (12 * j);
			byte_count = out(p, cursor, temp);
		}
	}

	// Pad to 4 byte boundary
	int pad = 0;
	fwrite(&pad, sizeof(char), 4 - (byte_count % 4), p);

	// And finally go back to write the size
	fseek(p, 4, SEEK_SET);
	byte_count += (4 - (byte_count % 4));
	byte_count = htobe32(byte_count);
	fwrite(&byte_count, sizeof(int), 1, p);
	/*
	printf("mesh count: %.2X @%02X\n", mesh_count, mesh_offset);
	printf("bone count: %.2X @%02X\n", bone_count, bone_offset);
	printf("tex count: %.2X @%02X\n", tex_count, texname_offset);
	printf("obj group count: %.2X @%02X\n", obj_group_count, obj_groupname_offset);
	*/
}

int main(int argc, char * argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s infile outfile\n", argv[0]);
        return 1;
    }

    FILE * yobj_file = fopen(argv[1], "rb");
    if (!yobj_file)
    {
        printf("Cannot open %s\n", argv[1]);
        return 1;
    }
    FILE * pof0_file = fopen(argv[2], "wb");
    if (!yobj_file)
    {
        printf("Cannot open %s\n", argv[2]);
        return 1;
    }
    generate_pof0(yobj_file, pof0_file);
    fclose(yobj_file);
    fclose(pof0_file);

    return 0;
}
