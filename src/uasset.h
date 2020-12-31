#include "core_types.h"
#include "serialize.h"

struct AssetHeader;

struct BaseCtx {
	bool useStringRef = true;
};

struct UnrealName {
	std::string name;
	i16 nonCasePreservingHash;
	i16 casePreservingHash;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(name);
		buffer.serialize(nonCasePreservingHash);
		buffer.serialize(casePreservingHash);
	}
};

struct Guid {
	char guid[16];

	void serialize(DataBuffer &buffer) {
		buffer.serialize(guid);
	}
};

struct Link {
	u64 base;
	u64 cls;
	i32 link;
	u64 property;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(base);
		buffer.serialize(cls);
		buffer.serialize(link);
		buffer.serialize(property);
	}
};


struct StringRef32 {
	i32 ref;
	std::string str;

	bool operator==(const StringRef32& rhs) {
		if (str.empty()) {
			return ref == rhs.ref;
		}
		else {
			return str == rhs.str;
		}
	}

	const std::string& getString(const AssetHeader &header);
	const std::string& getString(const AssetHeader *header) {
		if (header) {
			return getString(*header);
		}
		else {
			return str;
		}
	}

	void serialize(DataBuffer &buffer) {
		if (buffer.ctx<BaseCtx>().useStringRef) {
			buffer.serialize(ref);
		}
		else {
			buffer.serialize(str);
		}
	}
};

struct StringRef64 {
	i64 ref;
	std::string str;

	StringRef64() {}
	StringRef64(StringRef32 r) : ref(r.ref), str(r.str) {}

	const std::string& getString(const AssetHeader &header);
	const std::string& getString(const AssetHeader *header) {
		if (header) {
			return getString(*header);
		}
		else {
			return str;
		}
	}

	void serialize(DataBuffer &buffer) {
		if (buffer.ctx<BaseCtx>().useStringRef) {
			buffer.serialize(ref);
		}
		else {
			buffer.serialize(str);
		}
	}
};


struct Catagory {
	i32 connection;
	i32 connect;
	i32 link;
	i32 typeIdx;
	i64 garbageOne;
	i16 type;
	i16 garbageNew;
	i32 lengthV;
	i32 garbageTwo;
	i32 startV;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(connection);
		buffer.serialize(connect);
		buffer.serialize(link);
		buffer.serialize(typeIdx);
		buffer.serialize(garbageOne);
		buffer.serialize(type);
		buffer.serialize(garbageNew);
		buffer.watch([&]() { buffer.serialize(lengthV); });
		buffer.serialize(garbageTwo);
		buffer.watch([&]() { buffer.serialize(startV); });
	}
};

struct CatagoryRef {
	std::vector<i32> data;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(data);
	}
};

struct Version {
	u16 major;
	u16 minor;
	u16 patch;
	i32 changelist;
	std::string branch;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(major);
		buffer.serialize(minor);
		buffer.serialize(patch);
		buffer.serialize(changelist);
		buffer.serialize(branch);
	}
};

struct AssetHeader {
	u32 magic;
	i32 legacyFileVersion;
	i32 legacyUE3Version;
	i32 UE4FileVersion;
	i32 fileVersionLincenceeUE4;
	std::vector<i32> customVersions;
	i32 totalHeaderSize;
	std::string name;
	u32 packageFlags;
	i32 stringCount;
	char unk_buffer[20];
	i32 headerSize;
	i64 unk;
	i32 dataCategoryCount;
	i32 section3Offset;
	i32 sectionTwoLinkCount;
	i32 sectionTwoOffset;
	i32 sectionFourOffset;
	i32 sectionFiveStringCount;
	i32 sectionFiveOffset;

	i64 another_unk;
	Guid assetGUID;

	struct Generation {
		i32 exportCount;
		i32 nameCount;

		void serialize(DataBuffer &buffer) {
			buffer.serialize(exportCount);
			buffer.serialize(nameCount);
		}
	};
	std::vector<Generation> generations;

	Version savedByVersion;
	Version compatibleWithVersion;
	u32 compressionFlags;
	u32 packageSource;

	i32 hash;
	i32 unk_zero;
	i32 uexpDataOffset;
	i32 fileSize_minusFour;

	std::vector<UnrealName> names;
	std::vector<Link> links;
	std::vector<Catagory> catagories;
	char unk_buffer_2[64];
	std::vector<CatagoryRef> catagoryGroups;
	std::vector<std::string> section5Strings;

	u32 uexpDataCount;
	std::vector<i32> uexpData;

	u64 getLinkRef(i32 idx) const {
		if (idx < 0) {
			return links[-(idx + 1)].property;
		}
		else {
			return -idx;
		}
	}

	const std::string& getHeaderRef(i32 ref) const {
		static std::string BAD_STRING = "BAD";
		if (ref < 0) return BAD_STRING;
		if (ref > names.size()) BAD_STRING;
		return names[ref].name;
	}

	StringRef32 findOrCreateName(const std::string &str) {
		for (size_t i = 0; i < names.size(); ++i) {
			if (names[i].name == str) {
				StringRef32 r;
				r.ref = i;
				return r;
			}
		}

		auto idx = names.size();
		UnrealName name;
		name.name = str;
		names.emplace_back(name);
		stringCount = names.size();

		StringRef32 r;
		r.ref = idx;
		return r;
	}

	StringRef32 findName(const std::string &str) {
		for (size_t i = 0; i < names.size(); ++i) {
			if (names[i].name == str) {
				StringRef32 r;
				r.ref = i;
				return r;
			}
		}

		StringRef32 r;
		r.ref = std::numeric_limits<i32>::max();
		return r;
	}

	void serialize(DataBuffer &buffer) {
		if (generations.size() > 0) {
			generations[0].exportCount = dataCategoryCount;
			generations[0].nameCount = stringCount;
		}

		size_t start = 0;

		buffer.serialize(magic);
		if (magic != 0x9E2A83C1) {
			__debugbreak();
			return;
		}

		buffer.serialize(legacyFileVersion);
		buffer.serialize(legacyUE3Version);
		buffer.serialize(UE4FileVersion);
		buffer.serialize(fileVersionLincenceeUE4);
		buffer.serialize(customVersions);
		buffer.watch([&]() { buffer.serialize(totalHeaderSize); });
		buffer.serialize(name);
		buffer.serialize(packageFlags);
		buffer.serialize(stringCount);
		buffer.serialize(headerSize);
		buffer.serialize(unk);
		buffer.serialize(dataCategoryCount);
		buffer.watch([&]() { buffer.serialize(section3Offset); });
		buffer.serialize(sectionTwoLinkCount);
		buffer.watch([&]() { buffer.serialize(sectionTwoOffset); });
		buffer.watch([&]() { buffer.serialize(sectionFourOffset); });
		buffer.serialize(sectionFiveStringCount);
		buffer.watch([&]() { buffer.serialize(sectionFiveOffset); });
		buffer.serialize(another_unk);
		buffer.serialize(assetGUID);
		buffer.serialize(generations);

		buffer.serialize(savedByVersion);
		buffer.serialize(compatibleWithVersion);
		buffer.serialize(compressionFlags);
		buffer.serialize(packageSource);

		buffer.serialize(hash);
		buffer.serialize(unk_zero);

		buffer.watch([&]() { buffer.serialize(uexpDataOffset); });
		buffer.watch([&]() { buffer.serialize(fileSize_minusFour); });

		buffer.serialize(unk_buffer);

		auto jumpOrSetOffset = [&buffer](auto &pos) {
			if (buffer.loading) {
				buffer.pos = pos;
			}
			else {
				pos = buffer.pos;
			}
		};

		jumpOrSetOffset(headerSize);
		buffer.serializeWithSize(names, stringCount);

		jumpOrSetOffset(sectionTwoOffset);
		buffer.serializeWithSize(links, sectionTwoLinkCount);

		jumpOrSetOffset(section3Offset);
		buffer.serializeWithSize(catagories, dataCategoryCount);

		buffer.serialize(unk_buffer_2);

		jumpOrSetOffset(sectionFourOffset);
		buffer.serializeWithSize(catagoryGroups, dataCategoryCount);

		if (sectionFiveOffset != 0) {
			jumpOrSetOffset(sectionFiveOffset);
			buffer.serializeWithSize(section5Strings, sectionFiveStringCount);
		}

		if (totalHeaderSize > 0 && dataCategoryCount > 0) {
			jumpOrSetOffset(uexpDataOffset);

			if (buffer.loading) {
				uexpDataCount = (catagories[0].startV - buffer.pos) / sizeof(i32);
			}
			buffer.serializeWithSize(uexpData, uexpDataCount);
		}

		if (!buffer.loading) totalHeaderSize = buffer.pos;
	}
};

///////////////////////////////////////////////////////////////

struct AssetCtx {
	BaseCtx baseCtx;
	AssetHeader *header = nullptr;
	i64 length = 0;
	bool parseHeader = true;
	u32 headerSize = 0;
	bool parsingSaveFormat = false;
};

template<typename T>
struct PrimitiveProperty {
	T data;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(data);
	}
};

struct TextProperty {
	i32 flag;
	i8 historyType;
	u64 extras;
	std::vector<std::string> strings;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(flag);
		buffer.serialize(historyType);

		if (historyType == -1) {
			i32 num = extras;
			buffer.serialize(num);
			extras = num;
			buffer.serializeWithSize(strings, num);
		}
		else if (historyType == 0) {
			buffer.serializeWithSize(strings, 3);
		}
		else if (historyType = 11) {
			buffer.serialize(extras);
			buffer.serializeWithSize(strings, 1);
		}
	}
};

struct StringProperty {
	std::string str;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(str);
	}
};

struct ObjectProperty {
	i32 linkVal;

	StringRef32 type;
	StringRef32 value;
	void serialize(DataBuffer &buffer) {
		buffer.serialize(linkVal);

		if (buffer.ctx<AssetCtx>().parsingSaveFormat) {
			buffer.serialize(type);
			buffer.serialize(value);
		}
	}
};

struct EnumProperty {
	static const bool custom_header = true;

	StringRef64 enumType;
	u8 blank;
	StringRef64 value;


	void serialize(DataBuffer &buffer) {
		if (buffer.ctx<AssetCtx>().parseHeader) {
			size_t headerStart = buffer.pos;

			buffer.serialize(enumType);
			buffer.serialize(blank);

			buffer.ctx<AssetCtx>().headerSize += (buffer.pos - headerStart);
		}

		buffer.serialize(value);
	}
};

struct NameProperty {
	StringRef32 name;
	u32 v;

	void serialize(DataBuffer &buffer) {
		if (!buffer.ctx<AssetCtx>().parsingSaveFormat) {
			buffer.serialize(name);
			buffer.serialize(v);
		}
		else {
			buffer.serialize(name);
		}
	}
};

struct IPropertyValue;

struct ArrayProperty {
	ArrayProperty() {}
	ArrayProperty(ArrayProperty &&rhs) {
		arrayType = std::move(rhs.arrayType);
		values = std::move(rhs.values);
		rhs.values.clear();
	}
	ArrayProperty& operator=(ArrayProperty&& rhs) {
		arrayType = std::move(rhs.arrayType);
		values = std::move(rhs.values);
		rhs.values.clear();
		return *this;
	}

	static const bool custom_header = true;
	StringRef64 arrayType;
	std::vector<IPropertyValue*> values;

	~ArrayProperty();
	void serialize(DataBuffer &buffer);
};

struct MapProperty {
	static const bool custom_header = true;
	StringRef64 keyType;
	StringRef64 valueType;

	struct MapPair {
		IPropertyValue* key;
		IPropertyValue* value;
	};
	std::vector<MapPair> map;

	void serialize(DataBuffer &buffer);
};

struct StructProperty {
	static const bool custom_header = true;
	static const bool needs_length = true;

	Guid guid;
	StringRef64 type;
	std::vector<IPropertyValue*> values;

	void serialize(DataBuffer &buffer);
	IPropertyValue *get(const std::string &name);
};

struct ByteProperty {
	static const bool custom_header = true;

	StringRef64 enumType;
	i32 byteType;
	i32 value;

	void serialize(DataBuffer &buffer) {
		if (buffer.ctx<AssetCtx>().parseHeader) {
			size_t headerStart = buffer.pos;

			buffer.serialize(enumType);
			u8 null;
			buffer.serialize(null);

			buffer.ctx<AssetCtx>().headerSize += (headerStart - buffer.pos);
		}

		if (buffer.ctx<AssetCtx>().length == 1) {
			u8 v = value;
			buffer.serialize(v);
			value = v;
		}
		else if (buffer.ctx<AssetCtx>().length == 8 || buffer.ctx<AssetCtx>().length == 0) {
			u64 v = value;
			buffer.serialize(v);
			value = v;
		}
	}
};

struct SoftObjectProperty {
	StringRef32 name;
	u64 value;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(name);

		if (buffer.ctx<AssetCtx>().parsingSaveFormat) {
			u32 smolValue = value;
			buffer.serialize(smolValue);
			value = smolValue;
		}
		else {
			buffer.serialize(value);
		}
	}
};

struct BoolProperty {
	static const bool custom_header = true;
	bool value;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(value);

		if (buffer.ctx<AssetCtx>().parseHeader) {
			u8 null = 0;
			buffer.serialize(null);
		}
	}
};

struct DateTime {
	u64 time;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(time);
	}
};

struct IPropertyDataList;

struct UnknownProperty {
	static const bool needs_length = true;

	std::vector<u8> data;

	void serialize(DataBuffer &buffer) {
		buffer.serializeWithSize(data, (size_t)buffer.ctx<AssetCtx>().length);
	}
};

namespace asset_helper {
	using PropertyValue = std::variant<UnknownProperty, BoolProperty, PrimitiveProperty<i8>, PrimitiveProperty<i16>, PrimitiveProperty<i32>, PrimitiveProperty<i64>, PrimitiveProperty<u16>, PrimitiveProperty<u32>, PrimitiveProperty<u64>, PrimitiveProperty<float>,
									   TextProperty, StringProperty, ObjectProperty, EnumProperty, ByteProperty, NameProperty, ArrayProperty, MapProperty, StructProperty, PrimitiveProperty<Guid>, SoftObjectProperty, IPropertyDataList*, DateTime>;

	PropertyValue createPropertyValue(const std::string &type, const bool useUnknown = true);
	std::string getTypeForValue(const PropertyValue &v);

	void serialize(DataBuffer &buffer, i64 length, PropertyValue &value);

	bool needsLength(const PropertyValue &value);

	StringRef32 createNoneRef(DataBuffer &buffer);
}

struct IPropertyValue {
	asset_helper::PropertyValue v;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PropertyData {
	StringRef32 nameRef;
	i32 widgetData = 0;
	StringRef64 typeRef;
	i64 length = 0;
	asset_helper::PropertyValue value;
	bool isNone = false;

	void serialize(DataBuffer &buffer) {
		auto headerPtr = buffer.ctx<AssetCtx>().header;

		buffer.serialize(nameRef);

		if (!buffer.ctx<AssetCtx>().parsingSaveFormat) {
			buffer.serialize(widgetData);

			if (nameRef.getString(*headerPtr) == "None" || nameRef.ref == 0) {
				isNone = true;
				return;
			}
		}
		else {
			if (nameRef.str == "None") {
				isNone = true;
				return;
			}
		}

		buffer.serialize(typeRef);
		buffer.serialize(length);


		if (buffer.loading) {
			std::string type;

			if (!buffer.ctx<AssetCtx>().parsingSaveFormat) {
				type = nameRef.getString(*headerPtr);
				if (typeRef.ref > 0) {
					type = typeRef.getString(*headerPtr);
				}
			}
			else {
				type = typeRef.str;
			}

			value = asset_helper::createPropertyValue(type);
		}

		size_t beforeProp = buffer.pos;
		auto prevSize = buffer.ctx<AssetCtx>().headerSize;

		buffer.ctx<AssetCtx>().headerSize = 0;
		asset_helper::serialize(buffer, length, value);

		if (!buffer.loading && asset_helper::needsLength(value)) {
			size_t finalPos = buffer.pos;
			length = (finalPos - beforeProp) - buffer.ctx<AssetCtx>().headerSize;
			buffer.pos = beforeProp - sizeof(length);
			buffer.serialize(length);
			buffer.pos = finalPos;

		}

		buffer.ctx<AssetCtx>().headerSize = prevSize;
	}
};

struct IPropertyDataList {
	std::vector<PropertyData> properties;

	PropertyData* get(StringRef32 name) {
		for (auto &&p : properties) {
			if (p.nameRef == name) {
				return &p;
			}
		}

		return nullptr;
	}

	void serialize(DataBuffer &buffer) {
		bool parseHeader = buffer.ctx<AssetCtx>().parseHeader;
		buffer.ctx<AssetCtx>().parseHeader = true;

		if (buffer.loading) {
			bool keepParsing = true;
			while(buffer.pos < buffer.size - 4) {
				PropertyData data;
				buffer.serialize(data);

				if (!data.isNone) {
					properties.emplace_back(std::move(data));
				}
				else {
					break;
				}
			}
		}
		else {
			for (auto &&p : properties) {
				buffer.serialize(p);
			}

			StringRef32 NoneRef = asset_helper::createNoneRef(buffer);
			buffer.serialize(NoneRef);

			i32 null = 0;
			buffer.serialize(null);
		}

		buffer.ctx<AssetCtx>().parseHeader = parseHeader;
	}
};

struct NormalCategory {
	IPropertyDataList data;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(data);
	}
};

struct DataTableCategory {
	NormalCategory base;
	StringRef32 dataType;

	struct Entry {
		StringRef32 rowName;
		i32 duplicateId;
		StructProperty value;

		void serialize(DataBuffer &buffer) {
			buffer.serialize(rowName);
			buffer.serialize(duplicateId);
			buffer.serialize(value);
		}
	};

	std::vector<Entry> entries;

	void serialize(DataBuffer &buffer) {
		auto &&header = *buffer.ctx<AssetCtx>().header;
		buffer.serialize(base);

		
		if(buffer.loading) {
			for (auto &&p : base.data.properties) {
				if (p.nameRef.getString(header) == "RowStruct") {
					if (auto objPtr = std::get_if<ObjectProperty>(&p.value)) {
						dataType.ref = header.getLinkRef(objPtr->linkVal);
						break;
					}
				}
			}
		}

		i32 null = 0;
		buffer.serialize(null);

		i32 size = entries.size();
		buffer.serialize(size);

		bool parseHeader = buffer.ctx<AssetCtx>().parseHeader;
		buffer.ctx<AssetCtx>().parseHeader = false;
		buffer.ctx<AssetCtx>().length = 0;
		if (buffer.loading) {
			for (i32 i = 0; i < size; ++i) {
				Entry e;
				e.value.type.ref = dataType.ref;

				e.serialize(buffer);
				entries.emplace_back(std::move(e));
			}
		}
		else {
			buffer.serializeWithSize(entries, size);
		}
		buffer.ctx<AssetCtx>().parseHeader = parseHeader;
	}
};

struct AssetData {
	struct CatagoryValue {
		using CatagoryVariant = std::variant<NormalCategory, DataTableCategory>;

		CatagoryVariant value;
		std::vector<u8> extraData;
	};
	std::vector<CatagoryValue> catagoryValues;

	void serialize(DataBuffer &buffer) {
		auto &&header = *buffer.ctx<AssetCtx>().header;

		if (buffer.loading) {
			size_t catIdx = 0;
			for(auto &&c : header.catagories) {
				DataBuffer b = buffer.setupFromHere();
				b.size = c.lengthV;

				CatagoryValue v;

				std::string name = header.getHeaderRef(header.getLinkRef(c.connection));
				if (name == "DataTable") {
					DataTableCategory dataCat;
					b.serialize(dataCat);
					v.value = std::move(dataCat);
				}
				else {
					NormalCategory normalCat;
					b.serialize(normalCat);
					v.value = std::move(normalCat);
				}

				i32 nextStart = buffer.size - buffer.pos - 4;
				if (catIdx + 1 < header.catagories.size()) {
					nextStart = header.catagories[catIdx + 1].startV;
				}

				i32 extraLen = nextStart - b.pos;
				b.serializeWithSize(v.extraData, extraLen);

				catagoryValues.emplace_back(std::move(v));
				++catIdx;

				buffer.pos = b.pos + b.derivedBuffer->offset;
			}
		}
		else {
			size_t idx = 0;
			for (auto &&c : catagoryValues) {
				
				size_t start = buffer.pos;

				DataBuffer b = buffer.setupFromHere();
				std::visit([&](auto &&v) {
					b.serialize(v);
				}, c.value);

				header.catagories[idx].startV = start;
				header.catagories[idx].lengthV = b.size;

				buffer.pos = b.pos + b.derivedBuffer->offset;
				buffer.serializeWithSize(c.extraData, c.extraData.size());
				++idx;
			}

			i32 footer = 0x9E2A83C1;
			buffer.serialize(footer);
		}
	}
};

struct Asset {
	AssetHeader header;
	AssetData data;

	void serialize(DataBuffer &buffer) {
		AssetCtx ctx;
		ctx.parseHeader = true;
		ctx.header = &header;
		buffer.ctx_ = &ctx;

		buffer.serialize(header);
		buffer.serialize(data);

		header.fileSize_minusFour = buffer.size - 4;
	}
};




struct SaveFile {
	char start[48];
	std::string structName;
	IPropertyDataList properties;

	void serialize(DataBuffer &buffer) {
		AssetCtx ctx;
		ctx.baseCtx.useStringRef = false;
		ctx.parsingSaveFormat = true;

		buffer.ctx_ = &ctx;
		buffer.serialize(start);
		buffer.serialize(structName);
		buffer.serialize(properties);
	}
};


struct FSHAHash {
	char data[20];

	void serialize(DataBuffer &buffer) {
		buffer.serialize(data);
	}
};

enum class EPakVersion : u32
{
	INITIAL = 1,
	NO_TIMESTAMPS = 2,
	COMPRESSION_ENCRYPTION = 3,         // UE4.13+
	INDEX_ENCRYPTION = 4,               // UE4.17+ - encrypts only pak file index data leaving file content as is
	RELATIVE_CHUNK_OFFSETS = 5,         // UE4.20+
	DELETE_RECORDS = 6,                 // UE4.21+ - this constant is not used in UE4 code
	ENCRYPTION_KEY_GUID = 7,            // ... allows to use multiple encryption keys over the single project
	FNAME_BASED_COMPRESSION_METHOD = 8, // UE4.22+ - use string instead of enum for compression method
	FROZEN_INDEX = 9,
	PATH_HASH_INDEX = 10,
	FNV64BUGFIX = 11,


	LAST,
	INVALID,
	LATEST = LAST - 1
};

struct PakFile {
	struct Info {
		static const u32 OFFSET = 221;

		Guid guid;
		bool isEncrypted;
		u32 magic;
		EPakVersion version;
		i64 indexOffset;
		i64 indexSize;
		FSHAHash hash;
		bool isFrozen = false;
		char compressionName[32];

		void serialize(DataBuffer &buffer) {
			size_t start = buffer.pos;

			if (buffer.loading) {
				buffer.pos = buffer.size - OFFSET;
			}

			buffer.serialize(guid);
			buffer.serialize(isEncrypted);
			buffer.serialize(magic);
			if (magic != 0x5A6F12E1) {
				return;
			}

			buffer.serialize(version);
			buffer.watch([&]() { buffer.serialize(indexOffset); });
			buffer.watch([&]() { buffer.serialize(indexSize); });
			buffer.serialize(hash);

			if (version == EPakVersion::FROZEN_INDEX) {
				buffer.serialize(isFrozen);
			}

			buffer.serialize(compressionName);

			if (!buffer.loading) {
				std::vector<u8> nullData;
				nullData.resize(OFFSET - (buffer.pos - start));
				buffer.serialize(nullData.data(), nullData.size());
			}
		}
	};

	struct PakEntry {
		std::string name;

		struct EntryData {
			i64 offset;
			i64 size;
			i64 uncompressedSize;
			i32 compressionMethodIdx;
			FSHAHash hash;

			u8 flags;
			u32 compressionBlockSize;
			

			// Flags for serialization (volatile)
			bool inFilePrefix = false;
			//

			void serialize(DataBuffer &buffer) {
				if (inFilePrefix) {
					i64 null = 0;
					buffer.serialize(null);
				}
				else {
					buffer.watch([&]() { buffer.serialize(offset); });
				}

				buffer.watch([&]() { buffer.serialize(size); });
				buffer.watch([&]() { buffer.serialize(uncompressedSize); });
				buffer.serialize(compressionMethodIdx);
				buffer.watch([&]() { buffer.serialize(hash); });
				if (compressionMethodIdx != 0) {

				}
				buffer.serialize(flags);
				buffer.serialize(compressionBlockSize);
			}
		};
		EntryData entryData;

		struct PakAssetData {
			AssetHeader *header;
			AssetData data;
			size_t size;

			void serialize(DataBuffer &buffer) {
				AssetCtx ctx;
				ctx.header = header;
				buffer.ctx_ = &ctx;
				buffer.serialize(data);

				header->fileSize_minusFour = buffer.size + header->totalHeaderSize - 4;
				size = buffer.size;

				for (auto &&c : header->catagories) {
					c.startV += header->totalHeaderSize;
				}
			}
		};

		std::variant<AssetHeader, PakAssetData> data;
		

		void serialize(DataBuffer &buffer) {
			buffer.serialize(name);

			size_t start = buffer.pos;
			buffer.serialize(entryData);
			u32 structOffset = buffer.pos - start;

			if (buffer.loading) {
				size_t currentPos = buffer.pos;

				if (name.find(".uasset") != std::string::npos) {
					DataBuffer assetBuffer;
					assetBuffer.buffer = buffer.buffer + entryData.offset + structOffset;
					assetBuffer.size = entryData.uncompressedSize;

					AssetHeader header;
					assetBuffer.serialize(header);
					data = header;
				}
				else if (name.find(".uexp") != std::string::npos) {
					auto searchStr = name.substr(0, name.size() - 5) + ".uasset";

					AssetHeader *foundHeader = nullptr;
					for (auto &&e : buffer.ctx<PakFile>().entries) {
						if (e.name == searchStr) {
							foundHeader = &std::get<AssetHeader>(e.data);
						}
					}

					if (foundHeader) {
						PakAssetData pakData;
						pakData.header = foundHeader;

						DataBuffer assetBuffer;
						assetBuffer.buffer = buffer.buffer + entryData.offset + structOffset;
						assetBuffer.size = entryData.uncompressedSize;
						assetBuffer.serialize(pakData);

						data = std::move(pakData);
					}
				}

				buffer.pos = currentPos;
			}
		}
	};


	Info info_footer;
	std::string mountPoint;
	std::vector<PakEntry> entries;

	void serialize(DataBuffer &buffer) {
		buffer.ctx_ = this;

		if (buffer.loading) {
			buffer.serialize(info_footer);
			buffer.pos = info_footer.indexOffset;

			buffer.serialize(mountPoint);
			buffer.serialize(entries);
		}
		else {
			for (auto &&e : entries) {
				e.entryData.inFilePrefix = true;
				buffer.serialize(e.entryData);
				e.entryData.inFilePrefix = false;

				std::visit([&](auto &&d) {
					DataBuffer b = buffer.setupFromHere();
					b.serialize(d);
					buffer.pos = b.pos + b.derivedBuffer->offset;
				}, e.data);

				if (auto asset = std::get_if<PakEntry::PakAssetData>(&e.data)) {
					e.entryData.size = asset->size;
					e.entryData.uncompressedSize = asset->size;
				}
			}

			buffer.serialize(mountPoint);
			buffer.serialize(entries);
			buffer.serialize(info_footer);
		}
	}
};