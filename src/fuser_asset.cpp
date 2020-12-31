#include "uasset.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "crc.h"
#include <optional>

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
		ImGui::Text("Value: %s", dispCtx.header->getHeaderRef(dispCtx.header->getLinkRef(v.linkVal)).c_str());
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

void display_category(NormalCategory &cat) {
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

void display_asset(Asset &asset) {
	ImGui::PushID(&asset);
	dispCtx.header = &asset.header;

	ImGui::Begin("Asset");

	if (ImGui::CollapsingHeader("Names")) {
		size_t i = 0;
		for (auto &&n : asset.header.names) {
			ImGui::InputText(("Name[" + std::to_string(i) + "]").c_str(), &n.name);
			++i;
		}
	}

	if (ImGui::CollapsingHeader("Categories")) {
		for (auto &&c : asset.data.catagoryValues) {
			ImSubregion _(&c);

			if (auto normCat = std::get_if<NormalCategory>(&c.value)) {
				display_category(*normCat);
			}
			else if (auto dataCat = std::get_if<DataTableCategory>(&c.value)) {
				display_category(*dataCat);
			}
		}
	}

	ImGui::End();

	ImGui::PopID();
}

//#define DO_SAVE_FILE
//#define DO_ASSET_FILE
#define DO_PAK_FILE
SaveFile f;

PakFile pak;

void display_savefile(SaveFile &f) {
	ImGui::Begin("Save File");

	display_property(&f.properties);

	ImGui::End();
}

std::vector<Asset> assets;

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
		//std::ifstream infile("DT_UnlocksSongs.uasset", std::ios_base::binary);
		//std::ifstream uexpfile("DT_UnlocksSongs.uexp", std::ios_base::binary);

		std::ifstream infile("Meta_dornthisway.uasset", std::ios_base::binary);
		std::ifstream uexpfile("Meta_dornthisway.uexp", std::ios_base::binary);

		std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
		fileData.insert(fileData.end(), std::istreambuf_iterator<char>(uexpfile), std::istreambuf_iterator<char>());

		DataBuffer dataBuf;
		dataBuf.setupVector(fileData);
		{
			Asset asset;
			dataBuf.serialize(asset);
			assets.emplace_back(std::move(asset));
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
			std::ifstream infile("unlock_songs_P.pak", std::ios_base::binary);
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

			std::ofstream outPak("out_test.pak", std::ios_base::binary);
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

				std::ofstream outPak("out_test.sig", std::ios_base::binary);
				outPak.write((char*)sigOutBuf.buffer, sigOutBuf.size);

				std::ifstream infile("unlock_songs_P.sig", std::ios_base::binary);
				std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
				DataBuffer sigDataBuf;
				sigDataBuf.setupVector(fileData);

				test_buffer(sigDataBuf, sigOutBuf);
			}

		}
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

#if 0
		if (auto cat = std::get_if<DataTableCategory>(&a.data.catagoryValues[0].value)) {
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
			std::get<EnumProperty>(std::get<IPropertyDataList*>(e.value.values[0]->v)->get(a.header.findName("unlockCategory"))->value).value = a.header.findOrCreateName("EUnlockCategory::DLC");
			cat->entries.emplace_back(std::move(e));
		}
#endif

#if 0
		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		assets.back().serialize(outBuf);
		outBuf.finalize();

		std::ofstream outAsset("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uasset", std::ios_base::binary);
		std::ofstream outUexp("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uexp", std::ios_base::binary);
		
		//std::ofstream outAsset("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/Gameplay/Meta/DT_UnlocksSongs.uasset", std::ios_base::binary);
		//std::ofstream outUexp("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/Gameplay/Meta/DT_UnlocksSongs.uexp", std::ios_base::binary);
		
		//std::ofstream outAsset("out.uasset", std::ios_base::binary);
		//std::ofstream outUexp("out.uexp", std::ios_base::binary);

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

	ImGui::End();
}