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

	size_t findOrCreateName(const std::string &str) {
		for (size_t i = 0; i < names.size(); ++i) {
			if (names[i].name == str) {
				return i;
			}
		}

		auto idx = names.size();
		UnrealName name;
		name.name = str;
		names.emplace_back(name);
		stringCount = names.size();

		return idx;
	}

	void serialize(DataBuffer &buffer) {
		sectionOneStringCountAgain = stringCount;
		dataCategoryCountAgain = dataCategoryCount;


		buffer.serialize(magic);
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

		if (!buffer.loading) totalHeaderSize = buffer.pos;
	}
};

///////////////////////////////////////////////////////////////

struct AssetCtx {
	AssetHeader *header;
	i64 length;
	bool parseHeader;
	u32 headerSize = 0;
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
	std::string str;

	void serialize(DataBuffer &buffer) {
		buffer.serialize(str);
	}
};

struct ObjectProperty {
	i32 linkVal;

	void serialize(DataBuffer &buffer) {
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

			buffer.ctx<AssetCtx>().headerSize += sizeof(enumType) + 1;
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

	void serialize(DataBuffer &buffer) {
		printf("UNIMPLEMENTED! %s\n", __FUNCTION__);
	}
};

struct StructProperty {
	static const bool custom_header = true;

	Guid guid;
	StringRef64 type;
	std::vector<IPropertyValue*> values;

	void serialize(DataBuffer &buffer);
};

struct ByteProperty {

	void serialize(DataBuffer &buffer) {
		printf("UNIMPLEMENTED! %s\n", __FUNCTION__);
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

struct IPropertyDataList;

struct UnknownProperty {
	std::vector<u8> data;

	void serialize(DataBuffer &buffer) {
		buffer.serializeWithSize(data, (size_t)buffer.ctx<AssetCtx>().length);
	}
};

namespace asset_helper {
	using PropertyValue = std::variant<UnknownProperty, BoolProperty, PrimitiveProperty<i8>, PrimitiveProperty<i16>, PrimitiveProperty<i32>, PrimitiveProperty<i64>, PrimitiveProperty<u16>, PrimitiveProperty<u32>, PrimitiveProperty<u64>, PrimitiveProperty<float>,
									   TextProperty, StringProperty, ObjectProperty, EnumProperty, ByteProperty, NameProperty, ArrayProperty, MapProperty, StructProperty, PrimitiveProperty<Guid>, SoftObjectProperty, IPropertyDataList*>;

	PropertyValue createPropertyValue(const std::string &type, const bool useUnknown = true);
	std::string getTypeForValue(const PropertyValue &v);

	template<class T, class = void>
	struct has_custom_header : std::false_type {};

	template<class T>
	struct has_custom_header<T, typename voider<decltype(T::custom_header)>::type> : std::true_type {};

	void serialize(DataBuffer &buffer, const AssetHeader &header, i64 length, PropertyValue &value);
}

struct IPropertyValue {
	asset_helper::PropertyValue v;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PropertyData {
	StringRef nameRef;
	i32 widgetData;
	i64 typeNum;
	i64 length;
	asset_helper::PropertyValue value;
	bool isNone = false;

	void serialize(DataBuffer &buffer) {
		auto &&header = *buffer.ctx<AssetCtx>().header;

		buffer.serialize(nameRef.ref);
		buffer.serialize(widgetData);

		if (nameRef.getString(header) == "None" || nameRef.ref == 0) {
			isNone = true;
			return;
		}

		buffer.serialize(typeNum);

		buffer.serialize(length);


		if (buffer.loading) {
			std::string type = nameRef.getString(header);
			if (typeNum > 0) {
				type = header.getHeaderRef((i32)typeNum);
			}

			value = asset_helper::createPropertyValue(type);
		}

		size_t beforeProp = buffer.pos;
		auto prevSize = buffer.ctx<AssetCtx>().headerSize;

		buffer.ctx<AssetCtx>().headerSize = 0;
		asset_helper::serialize(buffer, header, length, value);

		if (!buffer.loading) {
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

	void serialize(DataBuffer &buffer) {
		bool parseHeader = buffer.ctx<AssetCtx>().parseHeader;
		buffer.ctx<AssetCtx>().parseHeader = true;

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

			i32 NoneRef = buffer.ctx<AssetCtx>().header->findOrCreateName("None");
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
	StringRef dataType;

	struct Entry {
		StringRef rowName;
		i32 duplicateId;
		StructProperty value;

		void serialize(DataBuffer &buffer) {
			buffer.serialize(rowName.ref);
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
	using CatagoryValue = std::variant<NormalCategory, DataTableCategory>;
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
				if (name == "DataTable") {
					DataTableCategory dataCat;
					b.serialize(dataCat);
					catagoryValues.emplace_back(std::move(dataCat));
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
				
				DataBuffer b = buffer;
				std::vector<u8> data;
				b.setupVector(data);
				b.pos = 0;
				std::visit([&](auto &&v) {
					b.serialize(v);
				}, c);

				size_t start = buffer.pos;
				buffer.serialize(b.buffer, b.size);
				header.catagories[idx].startV = start;
				header.catagories[idx].lengthV = b.size;

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