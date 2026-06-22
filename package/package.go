package rmui

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	cfenc "github.com/jurgen-kluft/cfenc/package"
	rcore "github.com/jurgen-kluft/rcore/package"
	rlcd "github.com/jurgen-kluft/rlcd/package"
	rwifi "github.com/jurgen-kluft/rwifi/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rmui"
)

func GetPackage() *denv.Package {
	// dependencies
	corepkg := rcore.GetPackage()
	wifipkg := rwifi.GetPackage()
	lcdpkg := rlcd.GetPackage()
	fencpkg := cfenc.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)
	mainpkg.AddPackage(wifipkg)
	mainpkg.AddPackage(lcdpkg)
	mainpkg.AddPackage(fencpkg)

	// mui library
	mainlib := denv.SetupCppLibrary(mainpkg, "mui", "mui")
	mainlib.AddDependencies(corepkg.GetMainLib())
	mainlib.AddDependencies(wifipkg.GetMainLib())
	mainlib.AddDependency(lcdpkg.GetLibrary("library_wcs"))
	mainlib.AddDependencies(fencpkg.GetMainLib())

	// mui application
	mainapp := denv.SetupCppAppProjectForArduinoEsp32(mainpkg, repo_name, "main")
	mainapp.AddDependency(mainlib)

	mainpkg.AddMainApp(mainapp)
	mainpkg.AddMainLib(mainlib)
	return mainpkg
}
