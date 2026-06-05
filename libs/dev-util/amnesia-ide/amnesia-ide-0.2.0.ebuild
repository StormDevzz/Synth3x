# Copyright 2025 Synth3x
# Distributed under the terms of the GNU General Public License v2

EAPI=8

CRATES="
	eframe@0.27
	egui@0.27
	egui_extras@0.27
	serde@1.0
	serde_json@1.0
	libloading@0.8
"

inherit cargo

DESCRIPTION="AmnesiaIDE — Lightweight GUI IDE for C, C++, C#, Rust, and Assembly"
HOMEPAGE="https://github.com/StormDevzz/Synth3x"
SRC_URI="
	https://github.com/StormDevzz/Synth3x/archive/v${PV}.tar.gz -> ${P}.tar.gz
	$(cargo_crate_uris)
"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

DEPEND="
	>=dev-libs/glib-2.56
	media-libs/libglvnd
	x11-libs/libX11
	x11-libs/libXrandr
"
RDEPEND="${DEPEND}"

src_unpack() {
	cargo_src_unpack
}

src_compile() {
	cargo_src_compile --release
}

src_install() {
	cargo_src_install
	dobin target/release/amnesia-ide
}
