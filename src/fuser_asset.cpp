#include "uasset.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <optional>

struct ImSubregion {
	ImSubregion(void* p) {
		ImGui::PushID(p);
		ImGui::Indent();
	}

	~ImSubregion() {
		ImGui::PopID();
		ImGui::Unindent();
	}
};

void display_asset(Asset &asset) {
	ImGui::PushID(&asset);

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

			if (auto normCat = std::get_if<NormalCategory>(&c)) {
				if (ImGui::CollapsingHeader("Properties")) {
					for (auto &&p : normCat->data.properties) {
						ImSubregion __(&p);
						ImGui::Text("%s - %s", asset.header.getHeaderRef(p.nameRef.ref).c_str(), asset.header.getHeaderRef(p.typeNum).c_str());
						{
							ImGui::Indent();
							ImGui::Text("length: %d", p.length);
							ImGui::Text("widgetData: %d", p.widgetData);
							ImGui::Unindent();
						}
					}
				}
			}
		}
	}

	ImGui::End();

	ImGui::PopID();
}

std::vector<Asset> assets;

void window_loop() {
	ImGui::Begin("Fuser Custom Song");

	if (ImGui::Button("Load")) {
		std::ifstream infile("DT_UnlocksSongs.uasset", std::ios_base::binary);
		std::ifstream uexpfile("DT_UnlocksSongs.uexp", std::ios_base::binary);
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
		
		if (auto normCat = std::get_if<NormalCategory>(&a.data.catagoryValues[0])) {
			auto createProp = [&](const std::string &name, asset_helper::PropertyValue &&v, std::optional<u32> idx = std::nullopt) {
				PropertyData prop;
				prop.nameRef.ref = a.header.findOrCreateName(name);
				prop.widgetData = 0;
				prop.typeNum = a.header.findOrCreateName(asset_helper::getTypeForValue(v));
				prop.value = std::move(v);
				prop.length = 0;

				auto insertPos = normCat->data.properties.end();
				if (idx.has_value()) {
					insertPos = normCat->data.properties.begin() + idx.value();
				}

				normCat->data.properties.emplace(insertPos, std::move(prop));
			};

			//BoolProperty dlcProp;
			//dlcProp.value = true;
			//createProp("IsDLC", std::move(dlcProp), normCat->properties.size() - 1);

			//PrimitiveProperty<u32> steamAppId;
			//steamAppId.data = 0;
			//createProp("SteamAppId", std::move(steamAppId));

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

		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		assets.back().serialize(outBuf);
		outBuf.finalize();

		//std::ofstream outAsset("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uasset", std::ios_base::binary);
		//std::ofstream outUexp("D:/Mettra_User/Downloads/UnrealPakSwitchv6/UnrealPakSwitch/Fuser/Content/DLC/Songs/dornthisway/Meta_dornthisway.uexp", std::ios_base::binary);

		std::ofstream outAsset("out.uasset", std::ios_base::binary);
		std::ofstream outUexp("out.uexp", std::ios_base::binary);
		outAsset.write((char*)outBuf.buffer, a.header.catagories[0].startV);
		outUexp.write((char*)outBuf.buffer + a.header.catagories[0].startV, outBuf.size - a.header.catagories[0].startV);

		if (dataBuf.size != outBuf.size || !memcmp(dataBuf.buffer, outBuf.buffer, dataBuf.size)) {
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
	}

	for (auto &&a : assets) {
		display_asset(a);
	}

	ImGui::End();
}