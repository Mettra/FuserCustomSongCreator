#include "uasset.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "crc.h"
#include <optional>
#include <algorithm>
#include <array>

#include <filesystem>
namespace fs = std::filesystem;

#include "custom_song_pak_template.h"

#define NOMINMAX
#include <Windows.h>

void replace(u8* data, size_t size, const std::string &find, const std::string &replace) {
	if (find.size() != replace.size()) {
		printf("NOT THE SAME SIZE!\n");
		__debugbreak();
		return;
	}

	for (size_t i = 0; i < size; ++i) {
		bool found = true;
		for (size_t j = 0; j < find.size(); ++j) {
			if ((*(data + i + j) != *(find.data() + j))) {
				found = false;
				break;
			}
		}

		if (found) {
			memcpy(data + i, replace.data(), replace.size());
		}
	}
}

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

struct Logger {
	size_t _indent = 0;

	void indent() {
		for (size_t i = 0; i < _indent; ++i) {
			printf("  ");
		}
	}

	void print(const char *format, ...) {
		indent();

		va_list argptr;
		va_start(argptr, format);
		vfprintf(stderr, format, argptr);
		va_end(argptr);

		printf("\n");
	}

	void push(const char *name) {
		print(name);
		_indent += 1;
	}

	void pop() {
		_indent -= 1;
	}
};

Logger gLog;

struct LogSection {
	LogSection(const char *name) {
		gLog.push(name);
	}

	~LogSection() {
		gLog.pop();
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
//#define ONE_SHOT
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

	const std::string& getSuffix() {
		static std::string suffix_beat = "bt";
		static std::string suffix_bass = "bs";
		static std::string suffix_lead = "ld";
		static std::string suffix_loop = "lp";

		std::string typeStr;
		switch (value) {
		case Type::Beat:
			return suffix_beat;

		case Type::Bass:
			return suffix_bass;

		case Type::Lead:
			return suffix_lead;

		case Type::Loop:
			return suffix_loop;
		};

		static std::string null;
		return null;
	}

	std::string suffix(const std::string &name) {
		return name + getSuffix();
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

enum class MidiType {
	Major,
	Minor
};

struct SongSerializationCtx {
	bool loading = true;
	
	std::string shortName;
	std::string songName;
	CelType curType;
	MidiType curMidiType;
	bool isTransition = false;

	std::string folderRoot() {
		return "Audio/Songs/" + shortName + "/";
	}

	std::string subCelFolder() {
		return curType.suffix(shortName) + "/";
	}

	std::string subCelName() {
		return curType.suffix(shortName) + (isTransition ? "_trans" : "");
	}

	std::string midiSuffix() {
		if (isTransition) {
			return "";
		}

		return (curMidiType == MidiType::Major ? "_maj" : "_min");
	}

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

static const std::string Game_Prefix = "/Game/";

struct SongPakEntry {
	PakFile::PakEntry *e = nullptr;
	std::string path;
	std::string name;

	//@REVISIT: This is pretty janky, since exceptions need to be manually set. 
	//But I don't know if there's a better way to find this through the object structure.
	i32 thisObjectPath = 0;

	void serialize(SongSerializationCtx &ctx, const std::string &parentPath, const std::string &fileName) {
		if (ctx.loading) {
			e = ctx.getFile(parentPath + fileName);
		}
		else {
			auto &&header = e->getData().pakHeader->getHeader();

			path = parentPath + fileName;
			name = fileName;

			e->name = path + ".uexp";
			std::get<PakFile::PakEntry::PakAssetData>(e->data).pakHeader->name = path + ".uasset";

			header.names[header.catagories[0].objectName].name = fileName;

			if (thisObjectPath != -1) {
				header.names[thisObjectPath].name = Game_Prefix + parentPath + fileName;
			}
		}
	}
};

template<typename T>
struct FileLink {
	i64 linkVal;
	T data;

	void serialize(SongSerializationCtx &ctx) {
		auto prevFile = ctx.curEntry;
		auto &&header = ctx.getHeader();
		LogSection s(header.getHeaderRef(header.getLinkRef(header.getLinkRef(linkVal).link).property).c_str());

		if (ctx.loading) {
			
			auto &&linkedFile = header.getLinkRef(header.getLinkRef(linkVal).link);
			data.file.e = ctx.getFile(header.getHeaderRef(linkedFile.property).substr(Game_Prefix.size()) + ".uexp");
		}

		if (data.file.e == nullptr) return;
		ctx.curEntry = data.file.e;
		data.serialize(ctx);
		ctx.curEntry = prevFile;

		if (!ctx.loading) {
			ctx.getHeader().names[ctx.getHeader().getLinkRef(linkVal).property].name = fs::path(data.file.path).stem().string();

			auto &&linkedFile = header.getLinkRef(header.getLinkRef(linkVal).link);
			ctx.getHeader().names[linkedFile.property].name = Game_Prefix + data.file.path;
		}
	}
};

template<typename T>
struct AssetLink {
	T data;
	StringRef32 ref;
	std::optional<StringRef32> refWithoutExtension;
	bool hasExt = false;
	std::optional<StringRef32> fullRef;
	std::optional<StringRef32> shortRef;

	void serialize(SongSerializationCtx &ctx) {
		auto prevFile = ctx.curEntry;
		auto &&header = ctx.getHeader();
		LogSection s(header.getHeaderRef(ref.ref).c_str());

		if (ctx.loading) {
			std::string fullPath = ref.getString(header);
			auto pos = fullPath.rfind('.');
			std::string assetPath = fullPath.substr(0, pos);
			hasExt = pos != std::string::npos;
			if (hasExt) {
				size_t idx = 0;
				for (auto &&s : header.names) {
					if (s.name == assetPath) {
						refWithoutExtension = StringRef32();
						refWithoutExtension->ref = idx;
					}
					++idx;
				}
			}
			//std::string assetName = fullPath.substr(pos + 1);
			data.file.e = ctx.getFile(assetPath.substr(Game_Prefix.size()) + ".uexp");

			for (auto &&l : header.links) {
				if (l.link != 0 && header.names[header.getLinkRef(l.link).property].name == assetPath) {
					shortRef = StringRef32();
					shortRef->ref = l.property;
				}
			}
		}

		if (data.file.e == nullptr) return;
		ctx.curEntry = data.file.e;
		data.serialize(ctx);
		ctx.curEntry = prevFile;

		if (!ctx.loading) {
			auto &&assetData = std::get<PakFile::PakEntry::PakAssetData>(data.file.e->data);
			auto &&subHeader = std::get<AssetHeader>(assetData.pakHeader->data);
			
			std::string assetPath = Game_Prefix + data.file.path;
			
			//@TODO: Another special case for beats
			if (refWithoutExtension) {
				header.names[refWithoutExtension->ref].name = assetPath;
			}
			
			if (hasExt) {
				assetPath += "." + subHeader.getHeaderRef(subHeader.catagories[0].objectName);
			}
			header.names[ref.ref].name = assetPath;

			if (shortRef.has_value()) {
				std::string shortName = assetPath.substr(assetPath.find_last_of('/') + 1);
				header.names[shortRef->ref].name = shortName;
			}
		}
	}
};


//////////

struct MidiFileAsset {
	SongPakEntry file;

	void serialize(SongSerializationCtx &ctx) {

		if (!ctx.loading) {
			file.serialize(ctx, ctx.folderRoot() + ctx.subCelFolder() + "midi/", ctx.subCelName() + "_mid" + ctx.midiSuffix());

			auto &&hmxAsset = std::get<HmxAssetFile>(file.e->getData().data.catagoryValues[0].value);
			hmxAsset.originalFilename = file.name;
			hmxAsset.audio.audioFiles[0].fileName = Game_Prefix + file.path + ".mid";
		}
	}
};

struct FusionFileAsset {
	SongPakEntry file;

	void serialize(SongSerializationCtx &ctx) {

		if (!ctx.loading) {
			file.serialize(ctx, ctx.folderRoot() + ctx.subCelFolder() + "patches/", ctx.subCelName() + "_fusion");

			auto &&asset = std::get<HmxAssetFile>(file.e->getData().data.catagoryValues[0].value);
			asset.originalFilename = ctx.subCelName() + "_fusion";

			std::vector<HmxAudio::PackageFile*> moggFiles;
			HmxAudio::PackageFile *fusionFile;
			
			for (auto &&file : asset.audio.audioFiles) {
				if (file.fileType == "FusionPatchResource") {
					fusionFile = &file;
				}
				else if (file.fileType == "MoggSampleResource") {
					moggFiles.emplace_back(&file);
				}
			}

			for (auto &&f : moggFiles) {
				f->fileName = "C:/" + ctx.subCelName() + ctx.midiSuffix() + ".mogg"; //Yes, moggs require a drive (C:/) before it otherwise they won't load.
			}

			fusionFile->fileName = Game_Prefix + file.path + ".fusion";
			auto &&fusion = std::get< HmxAudio::PackageFile::FusionFileResource>(fusionFile->resourceHeader);
			auto map = fusion.nodes.getNode("keymap");

			size_t idx = 0;
			for (auto c : map.children) {
				auto nodes = std::get<hmx_fusion_nodes*>(c.value);
				nodes->getString("sample_path") = moggFiles[idx % moggFiles.size()]->fileName;
				++idx;
			}
		}
	}
};

struct MidiSongAsset {
	SongPakEntry file;
	bool major = false;

	AssetLink<MidiFileAsset> midiFile;
	AssetLink<FusionFileAsset> fusionFile;

	void serialize(SongSerializationCtx &ctx) {
		auto &&hmxAsset = std::get<HmxAssetFile>(file.e->getData().data.catagoryValues[0].value);
		auto &&midiMusic = std::get<HmxAudio::PackageFile::MidiMusicResource>(hmxAsset.audio.audioFiles[0].resourceHeader);

		if (ctx.loading) {
			auto fusionPath = midiMusic.patch_engine_path.str;
			fusionPath = fusionPath.substr(0, fusionPath.find_first_of('.'));

			auto midiPath = midiMusic.mid_engine_path.str;
			midiPath = midiPath.substr(0, midiPath.find_first_of('.'));

			auto &&header = file.e->getData().pakHeader->getHeader();
			for (auto &&l : header.links) {
				if (header.getHeaderRef(l.property) == fusionPath) {
					fusionFile.ref.ref = l.property;
				}

				if (header.getHeaderRef(l.property) == midiPath) {
					midiFile.ref.ref = l.property;
				}
			}

		}

		ctx.curMidiType = major ? MidiType::Major : MidiType::Minor;
		midiFile.serialize(ctx);
		fusionFile.serialize(ctx);

		if (!ctx.loading) {
			std::string fileName = ctx.subCelName() + "_midisong" + (ctx.curType.value == CelType::Type::Beat ? "" : (ctx.curMidiType == MidiType::Major ? "_maj" : "_min"));
			file.serialize(ctx, ctx.folderRoot() + ctx.subCelFolder(), fileName);

			hmxAsset.originalFilename = fileName;
			hmxAsset.audio.audioFiles[0].fileName = Game_Prefix + file.path + ".midisong";

			midiMusic.mid_engine_path.str = Game_Prefix + midiFile.data.file.path + ".mid";
			midiMusic.patch_engine_path.str = Game_Prefix + fusionFile.data.file.path + ".fusion";
			midiMusic.midisong_engine_path.str = Game_Prefix + file.path + ".midisong";
			midiMusic.midisong_name.str = fileName + ".midisong";
			midiMusic.root.children[0].getArray().children[1].getString().str = "midi/" + midiFile.data.file.name + ".mid";
			midiMusic.root.children[1].getArray().children[1].getArray().children[2].getArray().children[1].getString().str = "patches/" + fusionFile.data.file.name + ".fusion";
		}
	}
};

struct SongTransition {
	SongPakEntry file;

	std::string shortName;
	std::vector<AssetLink<MidiSongAsset>> majorAssets;
	std::vector<AssetLink<MidiSongAsset>> minorAssets;

	void serialize(SongSerializationCtx &ctx) {
		ctx.isTransition = true;

		shortName = ctx.curType.suffix(ctx.shortName) + "_trans";
		ctx.serializeName("ShortName", shortName);

		if (ctx.loading) {
			for (auto &&v : ctx.getProp<ArrayProperty>("MajorMidiSongAssets")->values) {
				AssetLink<MidiSongAsset> midiAsset;
				midiAsset.ref = std::get<SoftObjectProperty>(v->v).name;
				midiAsset.data.major = true;
				midiAsset.serialize(ctx);
				majorAssets.emplace_back(std::move(midiAsset));
			}

			if (ctx.curType.value != CelType::Type::Beat) {
				for (auto &&v : ctx.getProp<ArrayProperty>("MinorMidiSongAssets")->values) {
					AssetLink<MidiSongAsset> midiAsset;
					midiAsset.ref = std::get<SoftObjectProperty>(v->v).name;
					midiAsset.data.major = false;
					midiAsset.serialize(ctx);
					minorAssets.emplace_back(std::move(midiAsset));
				}
			}
		}

		if (!ctx.loading) {
			ctx.serializeText("Title", ctx.songName);

			for (auto &&a : majorAssets) a.serialize(ctx);
			for (auto &&a : minorAssets) a.serialize(ctx);

			file.serialize(ctx, ctx.folderRoot() + ctx.subCelFolder(), "Meta_" + ctx.subCelName());
		}

		ctx.isTransition = false;
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

		

		shortName = ctx.shortName + type.getSuffix();
		ctx.serializeName("ShortName", shortName);
		ctx.curType = type;

		//@REVISIT
		file.thisObjectPath = 4;
		songTransitionFile.data.file.thisObjectPath = 4;
		if (type.value == CelType::Type::Beat) {
			file.thisObjectPath = 2;
			songTransitionFile.data.file.thisObjectPath = 2;
		}

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

			//@TODO: Another special case for beats
			if (ctx.curType.value != CelType::Type::Beat) {
				for (auto &&v : ctx.getProp<ArrayProperty>("MinorMidiSongAssets")->values) {
					AssetLink<MidiSongAsset> midiAsset;
					midiAsset.ref = std::get<SoftObjectProperty>(v->v).name;
					midiAsset.data.major = false;
					midiAsset.serialize(ctx);
					minorAssets.emplace_back(std::move(midiAsset));
				}
			}
		}

		if (!ctx.loading) {
			ctx.serializeText("Title", ctx.songName);

			for (auto &&a : majorAssets) {
				a.serialize(ctx);
			}

			for (auto &&a : minorAssets) {
				a.serialize(ctx);
			}

			file.serialize(ctx, ctx.folderRoot() + ctx.subCelFolder(), "Meta_" + type.suffix(ctx.shortName));
		}


		for (auto &&a : majorAssets) {
			gLog.print("Major - %s", ctx.getHeader().names[a.ref.ref].name.c_str());
		}
		for (auto &&a : minorAssets) {
			gLog.print("Minor - %s", ctx.getHeader().names[a.ref.ref].name.c_str());
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

		//@REVISIT
		file.thisObjectPath = 4;

		file.serialize(ctx, "DLC/Songs/" + ctx.shortName + "/", "Meta_" + ctx.shortName);
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
			ctx.serializeName("SongShortName", shortName);

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

std::optional<std::string> OpenFile(LPCSTR filter) {
	CHAR szFileName[MAX_PATH];

	// open a file name
	OPENFILENAME ofn;
	ZeroMemory(&szFileName, sizeof(szFileName));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFileName;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFileName);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileNameA(&ofn)) {
		return std::string(ofn.lpstrFile);
	}

	return std::nullopt;
}


void display_fuser_assets() {
	ImGui::Begin("Fuser Assets");

	ImGui::InputText("Short Name", &mainFile.shortName);
	ImGui::InputText("Song Name", &mainFile.songName);
	ImGui::InputText("Artist Name", &mainFile.artistName);

	if (ImGui::Button("Replace Beat Fusion")) {
		auto beatFusion = OpenFile("Any File (*.*)\0*.*\0");
		if (beatFusion) {
			std::ifstream infile(*beatFusion, std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

			for (auto &&c : mainFile.celData) {
				if (c.data.type.value == CelType::Type::Beat) {
					auto &&fusionFile = c.data.majorAssets[0].data.fusionFile.data;
					auto &&asset = std::get<HmxAssetFile>(fusionFile.file.e->getData().data.catagoryValues[0].value);

					for (auto &&a : asset.audio.audioFiles) {
						if (a.fileType == "FusionPatchResource") {
							auto &&fusionResource = std::get<HmxAudio::PackageFile::FusionFileResource>(a.resourceHeader);
							fusionResource.nodes = hmx_fusion_parser::parseData(fileData);
						}
					}
					
				}
			}
		}
	}

	if (ImGui::Button("Replace Beat Mogg")) {
		auto beatFusion = OpenFile("Encrypted Mogg (*.mogg)\0*.mogg\0");
		if (beatFusion) {
			std::ifstream infile(*beatFusion, std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

			for (auto &&c : mainFile.celData) {
				if (c.data.type.value == CelType::Type::Beat) {
					auto &&fusionFile = c.data.majorAssets[0].data.fusionFile.data;
					auto &&asset = std::get<HmxAssetFile>(fusionFile.file.e->getData().data.catagoryValues[0].value);

					for (auto &&a : asset.audio.audioFiles) {
						if (a.fileType == "MoggSampleResource") {
							a.fileData = std::move(fileData);
						}
					}
				}
			}
		}
	}

	if (ImGui::Button("Replace Beat Midi")) {
		auto beatFusion = OpenFile("Any File (*.*)\0*.*\0");
		if (beatFusion) {
			std::ifstream infile(*beatFusion, std::ios_base::binary);
			std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

			for (auto &&c : mainFile.celData) {
				if (c.data.type.value == CelType::Type::Beat) {
					auto &&midiFile = c.data.majorAssets[0].data.midiFile.data;
					auto &&asset = std::get<HmxAssetFile>(midiFile.file.e->getData().data.catagoryValues[0].value);
					asset.audio.audioFiles[0].fileData = std::move(fileData);
				}
			}
		}
	}

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

bool test_buffer(const DataBuffer &in_buffer, const DataBuffer &out_buffer) {
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
		return false;
	}
	else {
		printf("IS SAME!\n");
		return true;
	}
}

void window_loop() {
	ImGui::Begin("Fuser Custom Song");

	auto press = ImGui::Button("Load");
#ifdef ONE_SHOT
	press = true;
#endif
	if (press) {

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
		DataBuffer dataBuf;
		dataBuf.buffer = custom_song_pak_template;
		dataBuf.size = sizeof(custom_song_pak_template);
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

		printf("LOADING:\n\n");

		SongSerializationCtx ctx;
		ctx.loading = true;
		ctx.pak = &songPakFile;
		mainFile.serialize(ctx);

		//mainFile.shortName = "completely_different_short_name";
		//mainFile.songName = "My Custom Song (Not All Star)";
		//mainFile.artistName = "Mettra";


#if 0
		printf("\n\nSaving:\n\n");

		ctx.loading = false;
		mainFile.serialize(ctx);

		printf("\n\n");

		//for (auto &&e : songPakFile.entries) {
		//	if (auto header = std::get_if<AssetHeader>(&e.data)) {
		//		for (auto &&n : header->names) {
		//			if (n.name.find("dllstar") != std::string::npos) {
		//				printf("Header %s has a dllstar!\n", e.name.c_str());
		//				break;
		//			}
		//		}
		//	}
		//}

		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		songPakFile.serialize(outBuf);
		outBuf.finalize();

		std::ofstream outPak("out.pak", std::ios_base::binary);
		outPak.write((char*)outBuf.buffer, outBuf.size);

		test_buffer(dataBuf, outBuf);
#endif

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

	auto save_press = ImGui::Button("Save");

#ifdef ONE_SHOT
	save_press = true;
#endif

	if (save_press) {
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

		DataBuffer testBuf;
		testBuf.setupVector(outData);
		testBuf.serialize(songPakFile);

		std::string basePath = "D:/Program Files (x86)/Steam/steamapps/common/Fuser/Fuser/Content/Paks/custom_songs/";

		std::ofstream outPak(basePath + mainFile.shortName + "_P.pak", std::ios_base::binary);
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

			std::ofstream outPak(basePath + mainFile.shortName + "_P.sig", std::ios_base::binary);
			outPak.write((char*)sigOutBuf.buffer, sigOutBuf.size);
		}
	}
#endif

	ImGui::End();


#ifdef ONE_SHOT
	exit(0);
#endif
}