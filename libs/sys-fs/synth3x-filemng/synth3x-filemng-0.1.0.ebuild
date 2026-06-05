# Copyright 2025 Synth3x
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="Synth3x File Manager — GTK3 file manager with drag-and-drop and CSS animations"
HOMEPAGE="https://github.com/StormDevzz/Synth3x"
SRC_URI="https://github.com/StormDevzz/Synth3x/archive/v${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

DEPEND="
	>=x11-libs/gtk+-3.22
"
RDEPEND="${DEPEND}"

src_compile() {
	emake -C Synth3x-FileMng
}

src_install() {
	dobin Synth3x-FileMng/fileman
	doman Synth3x-FileMng/man/fileman.1 2>/dev/null || true
}
