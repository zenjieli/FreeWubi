#pragma once

#include "engine_types.h"

#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx-utils/textformatflags.h>

class FcitxOutput : public IEngineOutput {
public:
    FcitxOutput(fcitx::InputContext *ic, int pageSize)
        : ic_(ic), pageSize_(pageSize) {}

    void commit(const std::string &text) override {
        ic_->commitString(text);
    }

    void setPreedit(const std::string &text) override {
        fcitx::Text clientPreedit(text, fcitx::TextFormatFlag::Underline);
        clientPreedit.setCursor(-1);
        ic_->inputPanel().setClientPreedit(clientPreedit);
        ic_->updatePreedit();
        ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }

    void clearPanel() override {
        ic_->inputPanel().reset();
        ic_->updatePreedit();
        ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }

    void setCandidates(
        const std::vector<CandidateEntry> &candidates) override {
        auto candList = std::make_unique<fcitx::CommonCandidateList>();
        candList->setPageSize(pageSize_);
        for (auto &c : candidates) {
            candList->append<fcitx::DisplayOnlyCandidateWord>(
                fcitx::Text(c.label));
        }
        if (!candidates.empty()) {
            candList->setGlobalCursorIndex(0);
        }
        ic_->inputPanel().setCandidateList(std::move(candList));
        ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }

    void updateStatus() override {
        ic_->updateUserInterface(fcitx::UserInterfaceComponent::StatusArea);
    }

private:
    fcitx::InputContext *ic_;
    int pageSize_;
};
