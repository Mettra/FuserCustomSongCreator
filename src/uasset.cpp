#include "uasset.h"
#include "imgui.h"
#include "imgui_stdlib.h"

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
					for (auto &&p : normCat->properties) {
						ImSubregion __(&p);
						ImGui::Text("%s - %s", asset.header.getHeaderRef(p.nameRef).c_str(), asset.header.getHeaderRef(p.typeNum).c_str());
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
		std::ifstream infile("Meta_dornthisway.uasset", std::ios_base::binary);
		std::ifstream uexpfile("Meta_dornthisway.uexp", std::ios_base::binary);
		std::vector<u8> fileData = std::vector<u8>(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());
		fileData.insert(fileData.end(), std::istreambuf_iterator<char>(uexpfile), std::istreambuf_iterator<char>());

		DataBuffer dataBuf;
		dataBuf.setupVector(fileData);
		
		Asset asset;
		dataBuf.serialize(asset);
		assets.emplace_back(std::move(asset));

		std::vector<u8> outData;
		DataBuffer outBuf;
		outBuf.setupVector(outData);
		outBuf.loading = false;
		assets.back().serialize(outBuf);
		outBuf.finalize();

		if (dataBuf.size != outBuf.size || !memcmp(dataBuf.buffer, outBuf.buffer, dataBuf.size)) {
			printf("NOT THE SAME!\n");
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