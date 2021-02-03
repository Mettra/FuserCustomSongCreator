#include "CCallbacks.h"
#include <fstream>

size_t mogg_read(void *ptr, size_t size, size_t nmemb, void *datasource) {
	return fread(ptr, size, nmemb, (FILE*)datasource);
}
int mogg_seek(void *datasource, ogg_int64_t offset, int whence) {
	return fseek((FILE*)datasource, offset, whence);
}
int mogg_close(void *datasource) {
	return fclose((FILE*)datasource);
}
long mogg_tell(void *datasource) {
	return ftell((FILE*)datasource);
}

ov_callbacks cCallbacks = {
	mogg_read,
	mogg_seek,
	mogg_close,
	mogg_tell
};

ov_callbacks cppCallbacks = {
	[](void *ptr, size_t size, size_t nmemb, void *datasource) -> size_t {
		auto *file = static_cast<std::ifstream*>(datasource);
		file->read((char*)ptr, size * nmemb);
		return file->gcount() / size;
	},
	[](void *datasource, ogg_int64_t offset, int whence) -> int {
		auto *file = static_cast<std::ifstream*>(datasource);
		if (file->eof()) file->clear();
		
		std::ios_base::seekdir way;
		switch (whence) {
			case SEEK_SET: way = file->beg; break;
			case SEEK_CUR: way = file->cur; break;
			case SEEK_END: way = file->end; break;
		}
		file->seekg(offset, way);
		return file->fail();
	},
	[](void *datasource) -> int {
		auto *file = static_cast<std::ifstream*>(datasource);
		file->close();
		return file->is_open() ? EOF : 0;
	},
	[](void *datasource) -> long {
		auto *file = static_cast<std::ifstream*>(datasource);
		return file->tellg();
	}
};