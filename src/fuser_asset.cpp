#include "uasset.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "crc.h"
#include <optional>
#include <algorithm>
#include <array>

#include <filesystem>
namespace fs = std::filesystem;

struct ImSubregion {
	ImSubregion(void* p) {
		ImGui::PushID(p);
		ImGui::Indent();
	}

	ImSubregion(size_t s) {
		ImGui::PushID(s);
		ImGui::Indent();
	}

	~ImSubregion() {
		ImGui::PopID();
		ImGui::Unindent();
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DisplayPropertyCtx {
	AssetHeader *header = nullptr;
};
DisplayPropertyCtx dispCtx;

void display_property(asset_helper::PropertyValue& v);

void display_property(StringRef32& v) {
	if (v.str.empty()) {
		ImGui::Text("%s", v.getString(dispCtx.header).c_str());
	}
	else {
		ImGui::Text("%s", v.str.c_str());
	}
}

void display_property(StringRef64& v) {
	if (v.str.empty()) {
		ImGui::Text("%s", v.getString(dispCtx.header).c_str());
	}
	else {
		ImGui::Text("%s", v.str.c_str());
	}
}

void display_property(UnknownProperty& v) {
	ImGui::Text("Unknown Property (Length %d)", v.data);
}

void display_property(BoolProperty& v) {
	ImGui::Checkbox("", &v.value);
}

template<typename T>
void display_property(PrimitiveProperty<T>& v) {
	ImGuiDataType type = ImGuiDataType_Double;
	if constexpr (std::is_same_v<T, i8>) {
		type = ImGuiDataType_S8;
	}
	else if constexpr (std::is_same_v<T, i16>) {
		type = ImGuiDataType_S16;
	}
	else if constexpr (std::is_same_v<T, i32>) {
		type = ImGuiDataType_S32;
	}
	else if constexpr (std::is_same_v<T, i64>) {
		type = ImGuiDataType_S64;
	}
	else if constexpr (std::is_same_v<T, u16>) {
		type = ImGuiDataType_U16;
	}
	else if constexpr (std::is_same_v<T, u32>) {
		type = ImGuiDataType_U32;
	}
	else if constexpr (std::is_same_v<T, u64>) {
		type = ImGuiDataType_U64;
	}
	else if constexpr (std::is_same_v<T, float>) {
		type = ImGuiDataType_Float;
	}
	else if constexpr (std::is_same_v<T, Guid>) {
		ImGui::Text("GUID");
	}

	ImGui::InputScalar("", type, &v.data);
}

void display_property(TextProperty& v) {
	for (auto &&s : v.strings) {
		ImGui::Text("%s", s.c_str());
	}
}

void display_property(StringProperty& v) {
	ImGui::Text("%s", v.str.c_str());
}

void display_property(ObjectProperty& v) {
	if (dispCtx.header != nullptr) {
		auto &&link = dispCtx.header->getLinkRef(v.linkVal);

		ImGui::Text("Value: %s", dispCtx.header->getHeaderRef(link.property).c_str());
		ImGui::Text("Path: %s", dispCtx.header->getHeaderRef(dispCtx.header->getLinkRef(link.link).property).c_str());
	}
	else {
		ImGui::Text("Type: "); ImGui::SameLine(); display_property(v.type);
		ImGui::Text("Value: "); ImGui::SameLine(); display_property(v.value);
	}
}

void display_property(EnumProperty& v) {
	ImGui::Text("Type: "); ImGui::SameLine(); display_property(v.enumType);
	ImGui::Text("Value: "); ImGui::SameLine(); display_property(v.value);
}

void display_property(ByteProperty& v) {
	ImGui::Text("Type: "); ImGui::SameLine(); display_property(v.enumType);
	ImGui::InputScalar("", ImGuiDataType_U8, &v.value);
}

void display_property(NameProperty& v) {
	display_property(v.name);
}

void display_property(IPropertyValue* v) {
	display_property(v->v);
}

void display_property(ArrayProperty& v) {
	ImGui::Text("Array Type: "); ImGui::SameLine(); display_property(v.arrayType);

	size_t idx = 0;
	for (auto &&v : v.values) {
		ImSubregion r(idx);
		display_property(v);
		ImGui::Dummy(ImVec2(0, 12));
		++idx;
	}
}

void display_property(MapProperty& v) {
	ImGui::Text("Key Type: "); ImGui::SameLine(); display_property(v.keyType);
	ImGui::Text("Value Type: "); ImGui::SameLine(); display_property(v.valueType);

	size_t idx = 0;
	for (auto &&v : v.map) {
		ImSubregion r(idx);
		display_property(v.key);
		display_property(v.value);
		ImGui::Dummy(ImVec2(0, 12));
		++idx;
	}
}

void display_property(StructProperty& v) {
	size_t idx = 0;
	for (auto &&v : v.values) {
		ImSubregion r(idx);
		display_property(v);
		ImGui::Dummy(ImVec2(0, 12));
		++idx;
	}
}

void display_property(PropertyData &v) {
	std::string name = v.nameRef.getString(dispCtx.header);
	if (ImGui::CollapsingHeader(name.c_str())) {
		ImGui::Text("Type:"); ImGui::SameLine(); display_property(v.typeRef);
		ImGui::Text("Length: %d", v.length);
		display_property(v.value);
	}
}

void display_property(IPropertyDataList* v) {
	size_t idx = 0;
	for (auto &&prop : v->properties) {
		ImSubregion r(idx);
		display_property(prop);
		++idx;
	}
}

void display_property(SoftObjectProperty& v) {
	ImGui::Text("Name:"); ImGui::SameLine(); display_property(v.name);
	ImGui::InputScalar("Value", ImGuiDataType_U64, &v.value);
}

void display_property(DateTime& v) {
	ImGui::InputScalar("", ImGuiDataType_U64, &v.time);
}

void display_property(asset_helper::PropertyValue& v) {
	std::visit([](auto &&v) {
		display_property(v);
	}, v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void display_category(UObject &cat) {
	if (ImGui::CollapsingHeader("Properties")) {
		for (auto &&p : cat.data.properties) {
			ImSubregion __(&p);
			display_property(p);
		}
	}
}

void display_category(DataTableCategory &cat) {
	display_category(cat.base);
	ImGui::Text("Type:"); ImGui::SameLine(); display_property(cat.dataType);

	for (auto &&e : cat.entries) {
		ImSubregion __(&e);
		std::string rowName = e.rowName.getString(dispCtx.header);
		if (ImGui::CollapsingHeader(rowName.c_str())) {
			display_property(e.value);
		}
	}
}

void display_category(HmxAssetFile &fusionAsset) {
	for (auto &&file : fusionAsset.audio.audioFiles) {
		if (ImGui::CollapsingHeader(file.fileName.c_str())) {
			ImGui::Text("Type: %s", file.fileType.c_str());
			ImGui::Text("Size: %d", file.totalSize);
			if (ImGui::Button("Save Asset")) {
				std::ofstream outPak("fusion_out.bin", std::ios_base::binary);
				outPak.write((char*)file.fileData.data(), file.fileData.size());
			}
		}
	}
}

void display_asset(AssetData &assetData, AssetHeader *header) {
	ImGui::PushID(&assetData);
	dispCtx.header = header;

	if (ImGui::CollapsingHeader("Names")) {
		size_t i = 0;
		for (auto &&n : header->names) {
			ImGui::InputText(("Name[" + std::to_string(i) + "]").c_str(), &n.name);
			++i;
		}
	}

	if (ImGui::CollapsingHeader("Categories")) {
		for (auto &&c : assetData.catagoryValues) {
			ImSubregion _(&c);

			if (auto normCat = std::get_if<UObject>(&c.value)) {
				display_category(*normCat);
			}
			else if (auto dataCat = std::get_if<DataTableCategory>(&c.value)) {
				display_category(*dataCat);
			}
			else if (auto fusionAsset = std::get_if<HmxAssetFile>(&c.value)) {
				display_category(*fusionAsset);
			}
		}
	}

	ImGui::PopID();
}

/*
bs = bass
bt = drums
ld = vocals
lp = inst
*/


//#define DO_SAVE_FILE
//#define DO_ASSET_FILE
//#define DO_PAK_FILE
#define DO_SONG_CREATION
SaveFile f;

PakFile pak;

void display_savefile(SaveFile &f) {
	ImGui::Begin("Save File");

	display_property(&f.properties);

	ImGui::End();
}


struct CelType {
	enum class Type {
		Beat,
		Bass,
		Loop,
		Lead
	};
	Type value;

	const std::string& getPostfix() {
		static std::string postfix_beat = "bt";
		static std::string postfix_bass = "bs";
		static std::string postfix_lead = "ld";
		static std::string postfix_loop = "lp";

		std::string typeStr;
		switch (value) {
		case Type::Beat:
			return postfix_beat;

		case Type::Bass:
			return postfix_bass;

		case Type::Lead:
			return postfix_lead;

		case Type::Loop:
			return postfix_loop;
		};

		static std::string null;
		return null;
	}

	std::string postfix(const std::string &name) {
		return getPostfix() + name;
	}

	const std::string &getEnumValue() {
		static std::string enum_beat = "ECelType::Beat";
		static std::string enum_bass = "ECelType::Bass";
		static std::string enum_lead = "ECelType::Lead";
		static std::string enum_loop = "ECelType::Loop";

		switch (value) {
		case Type::Beat:
			return enum_beat;

		case Type::Bass:
			return enum_bass;

		case Type::Lead:
			return enum_lead;

		case Type::Loop:
			return enum_loop;
		};

		static std::string null;
		return null;
	}
};


struct SongSerializationCtx {
	bool loading = true;
	
	std::string shortName;
	std::string songName;
	CelType curType;

	PakFile *pak = nullptr;
	PakFile::PakEntry *curEntry = nullptr;


	PakFile::PakEntry *getFile(const std::string &fullPath) {
		for (auto &&e : pak->entries) {
			if (auto data = std::get_if<PakFile::PakEntry::PakAssetData>(&e.data)) {
				if (e.name.find(fullPath) != std::string::npos) {
					return &e;
				}
			}
		}

		return nullptr;
	}

	AssetHeader &getHeader(PakFile::PakEntry *entry = nullptr) {
		if (entry == nullptr) entry = curEntry;

		auto &&assetData = std::get<PakFile::PakEntry::PakAssetData>(entry->data);
		return std::get<AssetHeader>(assetData.pakHeader->data);
	}

	template<typename T>
	T* getProp(PakFile::PakEntry *entry, const std::string &propName) {
		auto &&assetData = std::get<PakFile::PakEntry::PakAssetData>(entry->data);
		AssetHeader *header = &std::get<AssetHeader>(assetData.pakHeader->data);
		auto &&obj = std::get<UObject>(assetData.data.catagoryValues[0].value);

		auto v = obj.data.get(header, propName);
		if (!v) {
			return nullptr;
		}

		return &std::get<T>(v->value);
	}

	template<typename T>
	T* getProp(const std::string &propName) {
		return getProp<T>(curEntry, propName);
	}

	void serializeName(const std::string &propName, std::string &serializedStr) {
		auto &&prop = *getProp<NameProperty>(curEntry, propName);
		if (loading) {
			serializedStr = prop.name.getString(getHeader());
		}
		else {
			getHeader().names[prop.name.ref].name = serializedStr;
		}
	}

	void serializeString(const std::string &propName, std::string &serializedStr) {
		auto &&prop = *getProp<StringProperty>(curEntry, propName);
		if (loading) {
			serializedStr = prop.str;
		}
		else {
			prop.str = serializedStr;
		}
	}

	void serializeText(const std::string &propName, std::string &serializedStr) {
		auto &&prop = *getProp<TextProperty>(curEntry, propName);
		if (loading) {
			serializedStr = prop.strings.back();
		}
		else {
			prop.strings.back() = serializedStr;
		}
	}

	
};

struct SongPakEntry {
	PakFile::PakEntry *e = nullptr;
	std::string path;

	void serialize(SongSerializationCtx &ctx, const std::string &path) {
		if (ctx.loading) {
			e = ctx.getFile(path);
		}
		else {
			this->path = path;
			e->name = path + ".uexp";
			std::get<PakFile::PakEntry::PakAssetData>(e->data).pakHeader->name = path + ".uasset";
		}
	}
};

template<typename T>
struct FileLink {
	i64 linkVal;
	T data;

	void serialize(SongSerializationCtx &ctx) {
		static const std::string prefix = "/Game/";

		auto prevFile = ctx.curEntry;
		auto &&header = ctx.getHeader();

		if (ctx.loading) {
			
			auto &&linkedFile = header.getLinkRef(header.getLinkRef(linkVal).link);
			data.file.e = ctx.getFile(header.getHeaderRef(linkedFile.property).substr(prefix.size()) + ".uexp");
		}

		if (data.file.e == nullptr) return;
		ctx.curEntry = data.file.e;
		data.serialize(ctx);
		ctx.curEntry = prevFile;

		if (!ctx.loading) {
			ctx.getHeader().names[ctx.getHeader().getLinkRef(linkVal).property].name = fs::path(data.file.path).stem().string();

			auto &&linkedFile = header.getLinkRef(header.getLinkRef(linkVal).link);
			ctx.getHeader().names[linkedFile.property].name = prefix + data.file.path;
		}
	}
};

template<typename T>
struct AssetLink {
	T data;
	StringRef32 ref;

	void serialize(SongSerializationCtx &ctx) {
		static const std::string prefix = "/Game/";
		auto prevFile = ctx.curEntry;
		auto &&header = ctx.getHeader();

		if (ctx.loading) {
			std::string fullPath = ref.getString(header);
			auto pos = fullPath.rfind('.');
			std::string assetPath = fullPath.substr(0, pos);
			//std::string assetName = fullPath.substr(pos + 1);
			data.file.e = ctx.getFile(assetPath.substr(prefix.size()) + ".uexp");
		}

		if (data.file.e == nullptr) return;
		ctx.curEntry = data.file.e;
		data.serialize(ctx);
		ctx.curEntry = prevFile;

		if (!ctx.loading) {
			auto &&assetData = std::get<PakFile::PakEntry::PakAssetData>(data.file.e->data);
			auto &&subHeader = std::get<AssetHeader>(assetData.pakHeader->data);
			
			header.names[ref.ref].name = prefix + data.file.path + "." + std::get<HmxAssetFile>(assetData.data.catagoryValues[0].value).assetName.getString(subHeader);
		}
	}
};


//////////

struct MidiSongAsset {
	SongPakEntry file;
	bool major = false;

	void serialize(SongSerializationCtx &ctx) {

		if (!ctx.loading) {
			file.serialize(ctx, "Audio/Songs/" + ctx.shortName + "/" + ctx.curType.postfix(ctx.shortName) + "/" + ctx.curType.postfix(ctx.shortName) + "_midisong_" + (major ? "maj" : "min"));
		}
	}
};

struct SongTransition {
	SongPakEntry file;

	std::string shortName;

	void serialize(SongSerializationCtx &ctx) {

		shortName = ctx.curType.postfix(ctx.shortName) + "_trans";
		ctx.serializeName("ShortName", shortName);

		if (!ctx.loading) {
			ctx.serializeText("Title", ctx.songName);
			file.serialize(ctx, "Audio/Songs/" + ctx.shortName + "/" + ctx.curType.postfix(ctx.shortName) + "/" + "Meta_" + ctx.curType.postfix(ctx.shortName) + "_trans");
		}
	}
};

struct CelData {
	SongPakEntry file;

	CelType type;
	std::string shortName;
	FileLink<SongTransition> songTransitionFile;
	std::vector<AssetLink<MidiSongAsset>> majorAssets;
	std::vector<AssetLink<MidiSongAsset>> minorAssets;

	void serialize(SongSerializationCtx &ctx) {
		

		if (ctx.loading) {
			auto celType = ctx.getProp<EnumProperty>(ctx.curEntry, "CelType");
			if (celType == nullptr) {
				type.value = CelType::Type::Beat; //Beat is the first value, and therefore default of the enum, so it isn't serialized in the file.
			}
			else {
				auto celTypeStr = celType->value.getString(ctx.getHeader());
				if (celTypeStr == "ECelType::Bass") {
					type.value = CelType::Type::Bass;
				}
				else if (celTypeStr == "ECelType::Lead") {
					type.value = CelType::Type::Lead;
				}
				else if (celTypeStr == "ECelType::Loop") {
					type.value = CelType::Type::Loop;
				}
			}
		}

		

		shortName = ctx.shortName + type.getPostfix();
		ctx.serializeName("ShortName", shortName);
		ctx.curType = type;

		songTransitionFile.linkVal = ctx.getProp<ObjectProperty>("SongTransitionCelData")->linkVal;
		songTransitionFile.serialize(ctx);

		if (ctx.loading) {
			for (auto &&v : ctx.getProp<ArrayProperty>("MajorMidiSongAssets")->values) {
				AssetLink<MidiSongAsset> midiAsset;
				midiAsset.ref = std::get<SoftObjectProperty>(v->v).name;
				midiAsset.data.major = true;
				midiAsset.serialize(ctx);
				majorAssets.emplace_back(std::move(midiAsset));
			}

			for (auto &&v : ctx.getProp<ArrayProperty>("MinorMidiSongAssets")->values) {
				AssetLink<MidiSongAsset> midiAsset;
				midiAsset.ref = std::get<SoftObjectProperty>(v->v).name;
				midiAsset.data.major = false;
				midiAsset.serialize(ctx);
				minorAssets.emplace_back(std::move(midiAsset));
			}
		}

		if (!ctx.loading) {
			ctx.serializeText("Title", ctx.songName);

			for (auto &&a : majorAssets) a.serialize(ctx);
			for (auto &&a : minorAssets) a.serialize(ctx);

			file.serialize(ctx, "Audio/Songs/" + ctx.shortName + "/" + type.postfix(ctx.shortName) + "/" + "Meta_" + type.postfix(ctx.shortName));
		}
	}
};

struct MainFile {
	std::string shortName;
	std::string artistName;
	std::string songName;

	SongPakEntry file;
	
	std::vector<FileLink<CelData>> celData;

	void serialize(SongSerializationCtx &ctx) {
		ctx.shortName = shortName;
		ctx.songName = songName;

		file.serialize(ctx, "DLC/Songs/" + ctx.shortName + "/Meta_" + ctx.shortName);
		ctx.curEntry = file.e;

		ctx.serializeText("Artist", artistName);

		if (ctx.loading) {
			auto &celArray = *ctx.getProp<ArrayProperty>(file.e, "Cels");
			for (auto &&v : celArray.values) {
				FileLink<CelData> fileLink;
				fileLink.linkVal = std::get<ObjectProperty>(v->v).linkVal;
				fileLink.serialize(ctx);

				ctx.songName = songName = ctx.getProp<TextProperty>(fileLink.data.file.e, "Title")->strings.back();

				celData.emplace_back(std::move(fileLink));
			}
		}
		else {
			size_t idx = 0;
			for (auto &&e : celData) {
				e.serialize(ctx);
				++idx;
			}
		}
	}
};

PakFile songPakFile;
MainFile mainFile;


void display_fuser_assets() {
	ImGui::Begin("Fuser Assets");

	ImGui::InputText("Short Name", &mainFile.shortName);
	ImGui::InputText("Song Name", &mainFile.songName);
	ImGui::InputText("Artist Name", &mainFile.artistName);


	if (ImGui::CollapsingHeader("Raw Files")) {
		for (auto &&a : songPakFile.entries) {
			if (auto data = std::get_if<PakFile::PakEntry::PakAssetData>(&a.data)) {

				std::string title = "Asset - " + a.name;
				if (ImGui::CollapsingHeader(title.c_str())) {
					ImGui::Indent();
					display_asset(data->data, &std::get<AssetHeader>(data->pakHeader->data));
					ImGui::Unindent();
				}
			}
		}
	}

	ImGui::End();
}

void test_buffer(const DataBuffer &in_buffer, const DataBuffer &out_buffer) {
	if (in_buffer.size != out_buffer.size || memcmp(in_buffer.buffer, out_buffer.buffer, in_buffer.size) != 0) {
		printf("NOT THE SAME!\n");
		if (in_buffer.size != out_buffer.size) {
			printf("	Size mismatch! Original: %d; New: %d;\n", in_buffer.size, out_buffer.size);
		}

		size_t numPrint = 50;
		for (size_t i = 0; i < (in_buffer.size > out_buffer.size ? in_buffer.size : out_buffer.size); ++i) {
			if (in_buffer.buffer[i] != out_buffer.buffer[i]) {
				printf("Memory difference at location %d!\n", i);
				if (--numPrint == 0) {
					break;
				}
			}
		}
	}
	else {
		printf("IS SAME!\n");
	}
}

void window_loop() {
	ImGui::Begin("Fuser Custom Song");

	if (ImGui::Button("Load")) {

#ifdef DO_ASSET_FILE

#if 0
		{
			std::ifstream infile("Meta_dornthisway.uasset", std::ios_base::binary);
			std::ifstream uexpFile("Meta_dornthisway.uexp", std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
			fileData.insert(fileData.end(), std::istreambuf_iterator<char>(uexpFile), std::istreambuf_iterator<char>());

			Asset testAsset;
			DataBuffer dataBuf;
			dataBuf.setupVector(fileData);
			dataBuf.serialize(testAsset);
		}
#endif

		{

			for (auto& dirEntry : fs::recursive_directory_iterator(fs::current_path() / "template")) {
				if (dirEntry.is_regular_file() && dirEntry.path().extension().string() == ".uasset") {
					auto filename = dirEntry.path().stem().string();

					auto assetFile = dirEntry.path();
					auto uexpFile = dirEntry.path().parent_path() / (dirEntry.path().stem().string() + ".uexp");

					std::ifstream infile(assetFile, std::ios_base::binary);
					std::ifstream uexpfile(uexpFile, std::ios_base::binary);

					std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
					fileData.insert(fileData.end(), std::istreambuf_iterator<char>(uexpfile), std::istreambuf_iterator<char>());

					DataBuffer dataBuf;
					dataBuf.setupVector(fileData);

					FuserAsset fuserAsset;
					fuserAsset.name = filename;
					dataBuf.serialize(fuserAsset.assetData);
					assets.emplace_back(std::move(fuserAsset));
				}
			}
		}

		auto &&a = assets.back();
#endif

#ifdef DO_SAVE_FILE

		{
			std::ifstream infile("FuserSave", std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
			DataBuffer dataBuf;
			dataBuf.setupVector(fileData);
			dataBuf.serialize(f);
		}
#endif

#ifdef DO_PAK_FILE

		{
			std::ifstream infile("dllstar_template.pak", std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
			DataBuffer dataBuf;
			dataBuf.setupVector(fileData);
			dataBuf.serialize(pak);

			std::vector<u8> outData;
			DataBuffer outBuf;
			outBuf.setupVector(outData);
			outBuf.loading = false;
			pak.serialize(outBuf);
			outBuf.finalize();

			std::ofstream outPak("out.pak", std::ios_base::binary);
			outPak.write((char*)outBuf.buffer, outBuf.size);

			test_buffer(dataBuf, outBuf);


			{
				PakSigFile sigFile;
				sigFile.encrypted_total_hash.resize(512);

				const u32 chunkSize = 64 * 1024;
				for (size_t start = 0; start < outBuf.size; start += chunkSize) {
					size_t end = start + chunkSize;
					if (end > outBuf.size) {
						end = outBuf.size;
					}

					sigFile.chunks.emplace_back(CRC::MemCrc32(outBuf.buffer + start, end - start));
				}

				std::vector<u8> sigOutData;
				DataBuffer sigOutBuf;
				sigOutBuf.setupVector(sigOutData);
				sigOutBuf.loading = false;
				sigFile.serialize(sigOutBuf);
				sigOutBuf.finalize();

				std::ofstream outPak("out.sig", std::ios_base::binary);
				outPak.write((char*)sigOutBuf.buffer, sigOutBuf.size);
			}

		}
#endif

#ifdef DO_SONG_CREATION
		std::ifstream infile("dllstar_template.pak", std::ios_base::binary);
		std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
		DataBuffer dataBuf;
		dataBuf.setupVector(fileData);
		dataBuf.serialize(songPakFile);

		for (auto &&e : songPakFile.entries) {
			if (auto data = std::get_if<PakFile::PakEntry::PakAssetData>(&e.data)) {
				auto pos = e.name.find("DLC/Songs/");
				if (pos != std::string::npos) {
					std::string shortName;
					for (size_t i = 10; i < e.name.size(); ++i) {
						if (e.name[i] == '/') {
							break;
						}

						shortName += e.name[i];
					}

					//Double check we got the name correct
					if (e.name == ("DLC/Songs/" + shortName + "/Meta_" + shortName + ".uexp")) {
						mainFile.shortName = shortName;
						break;
					}
				}
			}
		}

		if (mainFile.shortName.empty()) {
			printf("FATAL ERROR! No short name detected!");
			__debugbreak();
		}

		SongSerializationCtx ctx;
		ctx.loading = true;
		ctx.pak = &songPakFile;
		mainFile.serialize(ctx);
#endif

#if 0
		if (auto normCat = std::get_if<NormalCategory>(&a.data.catagoryValues[0].value)) {
			auto createProp = [&](const std::string &name, asset_helper::PropertyValue &&v, std::optional<u32> idx = std::nullopt) {
				PropertyData prop;
				prop.nameRef = a.header.findOrCreateName(name);
				prop.widgetData = 0;
				prop.typeRef = a.header.findOrCreateName(asset_helper::getTypeForValue(v));
				prop.value = std::move(v);
				prop.length = 0;

				auto insertPos = normCat->data.properties.end();
				if (idx.has_value()) {
					insertPos = normCat->data.properties.begin() + idx.value();
				}

				normCat->data.properties.emplace(insertPos, std::move(prop));
			};

			//EnumProperty unlockProp;
			//unlockProp.enumType = a.header.findOrCreateName("EUnlockState");
			//unlockProp.value = a.header.findOrCreateName("EUnlockState::Unlocked");
			//createProp("unlockState", std::move(unlockProp), normCat->data.properties.size() - 1);

			//BoolProperty isDlc;
			//isDlc.value = true;
			//createProp("IsDlc", std::move(isDlc));

			/*
			StringProperty ps4;
			ps4.str = "";
			createProp("PS4EntitlementLabel", std::move(ps4));

			StructProperty xbox;
			xbox.type.ref = a.header.findOrCreateName("Guid");
			xbox.values.emplace_back(new PrimitiveProperty<Guid>());
			createProp("XboxOneProductId", std::move(xbox));

			PrimitiveProperty<i32> switchAoc;
			switchAoc.data = 0;
			createProp("SwitchAocIndex", std::move(switchAoc));

			StringProperty epicItem;
			epicItem.str = "";
			createProp("EpicItemId", std::move(epicItem));

			StringProperty epicOffer;
			epicOffer.str = "";
			createProp("EpicOfferId", std::move(epicOffer));

			StringProperty epicArtifact;
			epicArtifact.str = "";
			createProp("EpicArtifactId", std::move(epicArtifact));
			*/
		}
#endif

#if defined(DO_ASSET_FILE) && 0
		if (auto cat = std::get_if<DataTableCategory>(&a.data.catagoryValues[0].value)) {

#if 0
			DataBuffer entryCloneBuffer;
			AssetCtx ctx;
			ctx.header = &a.header;
			ctx.parseHeader = false;

			entryCloneBuffer.ctx_ = &ctx;
			entryCloneBuffer.loading = false;
			std::vector<u8> cloneData;
			entryCloneBuffer.setupVector(cloneData);

			cat->entries[0].serialize(entryCloneBuffer);

			DataTableCategory::Entry e = cat->entries[0];
			e.value.values.clear();

			entryCloneBuffer.pos = 0;
			entryCloneBuffer.loading = true;
			e.serialize(entryCloneBuffer);
			e.rowName = a.header.findOrCreateName("dornthisway");
			std::get<NameProperty>(std::get<IPropertyDataList*>(e.value.values[0]->v)->get(a.header.findName("unlockName"))->value).name.ref = e.rowName.ref;
			//std::get<EnumProperty>(std::get<IPropertyDataList*>(e.value.values[0]->v)->get(a.header.findName("unlockCategory"))->value).value = a.header.findOrCreateName("EUnlockCategory::DLC");
			std::get<PrimitiveProperty<i32>>(std::get<IPropertyDataList*>(e.value.values[0]->v)->get(a.header.findName("audioCreditsCost"))->value).data = 0;
			cat->entries.emplace_back(std::move(e));
#endif
		}
#endif

#if defined(DO_ASSET_FILE) && 0
		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		assets.back().serialize(outBuf);
		outBuf.finalize();

		//std::ofstream outAsset("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uasset", std::ios_base::binary);
		//std::ofstream outUexp("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uexp", std::ios_base::binary);
		
		//std::ofstream outAsset("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/Gameplay/Meta/DT_UnlocksSongs.uasset", std::ios_base::binary);
		//std::ofstream outUexp("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/Gameplay/Meta/DT_UnlocksSongs.uexp", std::ios_base::binary);
		
		std::ofstream outAsset("out.uasset", std::ios_base::binary);
		std::ofstream outUexp("out.uexp", std::ios_base::binary);

		outAsset.write((char*)outBuf.buffer, a.header.catagories[0].startV);
		outUexp.write((char*)outBuf.buffer + a.header.catagories[0].startV, outBuf.size - a.header.catagories[0].startV);

		if (dataBuf.size != outBuf.size || memcmp(dataBuf.buffer, outBuf.buffer, dataBuf.size) != 0) {
			printf("NOT THE SAME!\n");
			if (dataBuf.size != outBuf.size) {
				printf("	Size mismatch! Original: %d; New: %d;\n", dataBuf.size, outBuf.size);
			}

			size_t numPrint = 50;
			for (size_t i = 0; i < (dataBuf.size > outBuf.size ? dataBuf.size : outBuf.size); ++i) {
				if (dataBuf.buffer[i] != outBuf.buffer[i]) {
					size_t realPos = i;
					bool inUexp = false;
					if (realPos > a.header.catagories[0].startV) {
						realPos -= a.header.catagories[0].startV;
						inUexp = true;
					}

					printf("Memory difference at location %d! [%s]\n", realPos, inUexp ? "Uexp" : "Header");
					if (--numPrint == 0) {
						break;
					}
				}
			}
		}
		else {
			printf("IS SAME!\n");
		}
#endif 
	}

#ifdef DO_ASSET_FILE
	for (auto &&a : assets) {
		display_asset(a);
	}
#endif

#ifdef DO_SAVE_FILE
	display_savefile(f);
#endif

#ifdef DO_PAK_FILE
	display_savefile(f);
#endif

#ifdef DO_SONG_CREATION
	display_fuser_assets();

	if (ImGui::Button("Save")) {
		SongSerializationCtx ctx;
		ctx.loading = false;
		ctx.pak = &songPakFile;
		mainFile.serialize(ctx);

		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		songPakFile.serialize(outBuf);
		outBuf.finalize();

		std::ofstream outPak("out.pak", std::ios_base::binary);
		outPak.write((char*)outBuf.buffer, outBuf.size);

		{
			PakSigFile sigFile;
			sigFile.encrypted_total_hash.resize(512);

			const u32 chunkSize = 64 * 1024;
			for (size_t start = 0; start < outBuf.size; start += chunkSize) {
				size_t end = start + chunkSize;
				if (end > outBuf.size) {
					end = outBuf.size;
				}

				sigFile.chunks.emplace_back(CRC::MemCrc32(outBuf.buffer + start, end - start));
			}

			std::vector<u8> sigOutData;
			DataBuffer sigOutBuf;
			sigOutBuf.setupVector(sigOutData);
			sigOutBuf.loading = false;
			sigFile.serialize(sigOutBuf);
			sigOutBuf.finalize();

			std::ofstream outPak("out.sig", std::ios_base::binary);
			outPak.write((char*)sigOutBuf.buffer, sigOutBuf.size);
		}
	}
#endif

	ImGui::End();
}