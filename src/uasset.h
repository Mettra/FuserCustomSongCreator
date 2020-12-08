#include "core_types.h"
#include "serialize.h"

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

	i32 unk_one;
	i32 dataCategoryCountAgain;

	i32 sectionOneStringCountAgain;
	i32 hash;
	i32 unk_zero;
	i32 uexpDataOffset;
	i32 fileSize_minusFour;

	std::vector<UnrealName> names;
	std::vector<Link> links;
	std::vector<Catagory> catagories;
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

	std::string getHeaderRef(i32 ref) const {
		if (ref < 0) return std::to_string(-ref);
		if (ref > names.size()) return std::to_string(ref);
		return names[ref].name;
	}

	void serialize(DataBuffer &buffer) {
		buffer.serialize(magic);
		buffer.serialize(legacyFileVersion);
		buffer.serialize(legacyUE3Version);
		buffer.serialize(UE4FileVersion);
		buffer.serialize(fileVersionLincenceeUE4);
		buffer.serialize(customVersions);
		buffer.serialize(totalHeaderSize);
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
		buffer.serialize(unk_one);
		buffer.serialize(dataCategoryCountAgain);
		buffer.serialize(sectionOneStringCountAgain);
		char null_0[36] = { 0 };
		buffer.serialize(null_0);
		buffer.serialize(hash);
		buffer.serialize(unk_zero);
		buffer.watch([&]() { buffer.serialize(uexpDataOffset); });
		buffer.watch([&]() { buffer.serialize(fileSize_minusFour); });
		char null_1[20] = { 0 };
		buffer.serialize(null_1);

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

		char null_2[64] = { 0 };
		buffer.serialize(null_2);

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
	}
};

///////////////////////////////////////////////////////////////

struct AssetCtx {
	AssetHeader *header;
	i64 length;
	bool parseHeader;
};

template<typename T>
struct PrimitiveProperty {
	T data;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(data);
	}
};

struct StringRef {
	i32 ref;

	std::string getString(const AssetHeader &header) {
		return header.getHeaderRef(ref);
	}
};

struct StringRef64 {
	i64 ref;

	std::string getString(const AssetHeader &header) {
		return header.getHeaderRef(ref);
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

	void serialize(DataBuffer &buffer) {

	}
};

struct ObjectProperty {
	u8 null;
	i32 linkVal;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(null);
		buffer.serialize(linkVal);
	}
};

struct EnumProperty {
	static const bool custom_header = true;

	i64 enumType;
	u8 blank;
	StringRef64 value;


	void serialize(DataBuffer &buffer) {
		if (buffer.ctx<AssetCtx>().parseHeader) {
			buffer.serialize(enumType);
			buffer.serialize(blank);
		}

		buffer.serialize(value.ref);
	}
};

struct NameProperty {
	StringRef name;
	i32 v;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(name.ref);
		buffer.serialize(v);
	}
};

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
	std::vector<void*> values; //PropertyValue*

	~ArrayProperty();
	void serialize(DataBuffer &buffer);
};

struct MapProperty {

	void serialize(DataBuffer &buffer) {

	}
};

struct StructProperty {

	void serialize(DataBuffer &buffer) {

	}
};

struct ByteProperty {

	void serialize(DataBuffer &buffer) {

	}
};

struct SoftObjectProperty {
	StringRef name;
	u64 value;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(name.ref);
		buffer.serialize(value);
	}
};

struct UnknownProperty {
	std::vector<u8> data;

	void serialize(DataBuffer &buffer) {
		buffer.serializeWithSize(data, (size_t)buffer.ctx<AssetCtx>().length);
	}
};

namespace asset_helper {
	using PropertyValue = std::variant<UnknownProperty, PrimitiveProperty<bool>, PrimitiveProperty<i8>, PrimitiveProperty<i16>, PrimitiveProperty<i32>, PrimitiveProperty<i64>, PrimitiveProperty<u16>, PrimitiveProperty<u32>, PrimitiveProperty<u64>, PrimitiveProperty<float>,
									   TextProperty, StringProperty, ObjectProperty, EnumProperty, ByteProperty, NameProperty, ArrayProperty, MapProperty, StructProperty, PrimitiveProperty<Guid>, SoftObjectProperty>;

	PropertyValue createPropertyValue(const std::string &type) {
		if (type == "BoolProperty") {
			return PrimitiveProperty<bool>{};
		}
		else if (type == "Int8Property") {
			return PrimitiveProperty<i8>{};
		}
		else if (type == "Int16Property") {
			return PrimitiveProperty<i16>{};
		}
		else if (type == "IntProperty") {
			return PrimitiveProperty<i32>{};
		}
		else if (type == "Int64Property") {
			return PrimitiveProperty<i64>{};
		}
		else if (type == "UInt16Property") {
			return PrimitiveProperty<u16>{};
		}
		else if (type == "UIntProperty") {
			return PrimitiveProperty<u32>{};
		}
		else if (type == "UInt64Property") {
			return PrimitiveProperty<u64>{};
		}
		else if (type == "FloatProperty") {
			return PrimitiveProperty<float>{};
		}
		else if (type == "TextProperty") {
			return TextProperty{};
		}
		else if (type == "StrProperty") {
			return StringProperty{};
		}
		else if (type == "ObjectProperty") {
			return ObjectProperty{};
		}
		else if (type == "EnumProperty") {
			return EnumProperty{};
		}
		else if (type == "ByteProperty") {
			return ByteProperty{};
		}
		else if (type == "NameProperty") {
			return NameProperty{};
		}
		else if (type == "ArrayProperty") {
			return ArrayProperty{};
		}
		else if (type == "MapProperty") {
			return MapProperty{};
		}
		else if (type == "StructProperty") {
			return StructProperty{};
		}
		else if (type == "Guid") {
			return PrimitiveProperty<Guid>{};
		}
		else if (type == "SoftObjectProperty") {
			return SoftObjectProperty{};
		}
		else {
			return UnknownProperty{};
		}
	}

	template<class T, class = void>
	struct has_custom_header : std::false_type {};

	template<class T>
	struct has_custom_header<T, typename voider<decltype(T::custom_header)>::type> : std::true_type {};

	void serialize(DataBuffer &buffer, const AssetHeader &header, i64 length, PropertyValue &value) {
		buffer.ctx<AssetCtx>().length = length;

		std::visit([&](auto &&v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (!has_custom_header<T>::value) {
				if (buffer.ctx<AssetCtx>().parseHeader) {
					u8 dumbheader = 0;
					buffer.serialize(dumbheader);
				}
			}

			if constexpr (DataBuffer::has_serialize<T>::value) {
				buffer.serialize(v);
			}
			else {
				static_asset(false, "Unable to serialize!");
			}
		}, value);
	}
}

ArrayProperty::~ArrayProperty() {
	for (auto &&ptr : values) {
		asset_helper::PropertyValue *value = (asset_helper::PropertyValue *)ptr;
		delete value;
	}
}

void ArrayProperty::serialize(DataBuffer &buffer) {
	bool parseHeader = buffer.ctx<AssetCtx>().parseHeader;
	if(parseHeader) {
		buffer.serialize(arrayType.ref);

		u8 null = 0;
		buffer.serialize(null);
	}

	if (buffer.loading) {
		i32 size;
		buffer.serialize(size);

		values.resize(size);
		for (i32 i = 0; i < size; ++i) {
			asset_helper::PropertyValue *value = new asset_helper::PropertyValue();
			*value = asset_helper::createPropertyValue(arrayType.getString(*buffer.ctx<AssetCtx>().header));

			buffer.ctx<AssetCtx>().parseHeader = false;
			asset_helper::serialize(buffer, *buffer.ctx<AssetCtx>().header, 0, *value);
			buffer.ctx<AssetCtx>().parseHeader = parseHeader;

			values[i] = value;
		}
	}
	else {
		i32 size = values.size();
		buffer.serialize(size);

		for (auto &&ptr : values) {
			asset_helper::PropertyValue *value = (asset_helper::PropertyValue *)ptr;

			buffer.ctx<AssetCtx>().parseHeader = false;
			asset_helper::serialize(buffer, *buffer.ctx<AssetCtx>().header, 0, *value);
			buffer.ctx<AssetCtx>().parseHeader = parseHeader;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PropertyData {
	i32 nameRef;
	i32 widgetData;
	i64 typeNum;
	i64 length;
	asset_helper::PropertyValue value;
	bool isNone = false;

	void serialize(DataBuffer &buffer) {
		auto &&header = *buffer.ctx<AssetCtx>().header;

		buffer.serialize(nameRef);
		buffer.serialize(widgetData);

		if (header.getHeaderRef(nameRef) == "None" || nameRef == 0) {
			isNone = true;
			return;
		}

		buffer.serialize(typeNum);
		buffer.serialize(length);


		if (buffer.loading) {
			std::string type = header.getHeaderRef(nameRef);
			if (typeNum > 0) {
				type = header.getHeaderRef((i32)typeNum);
			}

			value = asset_helper::createPropertyValue(type);
		}

		asset_helper::serialize(buffer, header, length, value);
	}
};


struct NormalCategory {
	std::vector<PropertyData> properties;

	void serialize(DataBuffer &buffer) {
		if (buffer.loading) {
			bool keepParsing = true;
			do {
				PropertyData data;
				buffer.serialize(data);
				
				keepParsing = !data.isNone && (buffer.pos != buffer.size);
				if (keepParsing) {
					properties.emplace_back(std::move(data));
				}

			} while (keepParsing);
		}
		else {
			for (auto &&p : properties) {
				buffer.serialize(p);
			}

			i64 end = 0;
			buffer.serialize(end);
		}
	}
};

struct AssetData {
	using CatagoryValue = std::variant<NormalCategory>;
	std::vector<CatagoryValue> catagoryValues;

	void serialize(DataBuffer &buffer) {
		auto &&header = *buffer.ctx<AssetCtx>().header;

		if (buffer.loading) {
			for(auto &&c : header.catagories) {
				DataBuffer b;
				b.pos = 0;
				b.buffer = buffer.buffer + buffer.pos;
				b.size = c.lengthV;
				b.ctx_ = buffer.ctx_;

				std::string name = header.getHeaderRef(header.getLinkRef(c.connection));
				if (name == "") {

				}
				else {
					NormalCategory normalCat;
					b.serialize(normalCat);
					catagoryValues.emplace_back(std::move(normalCat));
				}
			}
		}
		else {
			size_t idx = 0;
			for (auto &&c : catagoryValues) {
				size_t start = buffer.pos;
				std::visit([&](auto &&v) {
					buffer.serialize(v);
				}, c);
				size_t end = buffer.pos;

				header.catagories[idx].startV = start;
				header.catagories[idx].lengthV = (end - start);

				i32 null = 0;
				buffer.serialize(null);

				++idx;
			}
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
	}
};