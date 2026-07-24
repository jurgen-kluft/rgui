package rgui

import (
	denv "github.com/jurgen-kluft/ccode/denv"
	ccova "github.com/jurgen-kluft/ccova/package"
	cgx2 "github.com/jurgen-kluft/cgx2/package"
	rcore "github.com/jurgen-kluft/rcore/package"
	rlcd "github.com/jurgen-kluft/rlcd/package"
	rwifi "github.com/jurgen-kluft/rwifi/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "rgui"
)

func GetPackage() *denv.Package {
	// dependencies
	corepkg := rcore.GetPackage()
	wifipkg := rwifi.GetPackage()
	lcdpkg := rlcd.GetPackage()
	ccovapkg := ccova.GetPackage()
	cgx2pkg := cgx2.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(corepkg)
	mainpkg.AddPackage(wifipkg)
	mainpkg.AddPackage(lcdpkg)
	mainpkg.AddPackage(ccovapkg)
	mainpkg.AddPackage(cgx2pkg)

	// gui library
	mainlib := denv.SetupCppLibrary(mainpkg, "gui", "gui")
	mainlib.AddDependencies(corepkg.GetMainLib())
	mainlib.AddDependencies(wifipkg.GetMainLib())
	mainlib.AddDependency(lcdpkg.GetLibrary("library_wcs"))
	mainlib.AddDependencies(ccovapkg.GetMainLib())
	mainlib.AddDependencies(cgx2pkg.GetMainLib())

	// gui application
	mainapp := denv.SetupCppAppProjectForArduinoEsp32(mainpkg, repo_name, "main")
	mainlib.AddDependency(lcdpkg.GetLibrary("library_touch"))
	mainapp.AddDependency(mainlib)

	mainpkg.AddMainApp(mainapp)
	mainpkg.AddMainLib(mainlib)
	return mainpkg
}
