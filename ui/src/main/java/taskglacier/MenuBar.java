package taskglacier;

import dialogs.*;
import io.github.andrewauclair.moderndocking.app.DockingState;
import io.github.andrewauclair.moderndocking.layouts.DockingLayouts;
import packets.BugzillaRefresh;
import packets.CreateTask;
import packets.RequestID;

import javax.swing.*;
import java.util.Arrays;
import java.util.List;

public class MenuBar extends JMenuBar {
    private final JMenuItem add;

    private final JMenuItem connect = new JMenuItem("Connect...");
    private final JMenuItem disconnect = new JMenuItem("Disconnect");
    private final JMenuItem requestDailyReport = new JMenuItem("Request Daily Report...");
    private final JMenuItem requestWeeklyReport = new JMenuItem("Request Weekly Report...");

    private final JMenu bugzilla = new JMenu("Bugzilla");

    public MenuBar(MainFrame mainFrame) {
        JMenu file = new JMenu("File");
        file.setMnemonic('F');

        JMenuItem hide = new JMenuItem("Hide");
        hide.addActionListener(e -> mainFrame.setVisible(false));
        file.add(hide);

        file.addSeparator();

        JMenuItem exit = new JMenuItem("Exit");
        exit.addActionListener(e -> System.exit(0));
        file.add(exit);

        add(file);

        JMenu task = new JMenu("Task");
        task.setMnemonic('T');

        add = new JMenuItem("Add...");
        add.setEnabled(false);
        add.addActionListener(e -> new AddModifyTask(mainFrame, mainFrame, 0, false).setVisible(true));
        task.add(add);

        JMenuItem timeEntry = new JMenuItem("Time Entry Configuration...");
        task.add(timeEntry);

        timeEntry.addActionListener(e -> new TimeEntryConfiguration(mainFrame).setVisible(true));

        add(task);


        JMenu server = new JMenu("Server");
        server.setMnemonic('S');

        connect.addActionListener(e -> {
            ConnectToServer connectToServer = new ConnectToServer(mainFrame);
            connectToServer.setVisible(true);
        });

        disconnect.addActionListener(e -> {
            mainFrame.disconnect();
        });
        disconnect.setEnabled(false);

        server.add(connect);
        server.add(disconnect);
        server.addSeparator();

        server.add(requestDailyReport);

        requestDailyReport.addActionListener(e -> {
            RequestDailyReportDialog dialog = new RequestDailyReportDialog(mainFrame);
            dialog.setVisible(true);
        });

        server.add(requestWeeklyReport);

        requestWeeklyReport.addActionListener(e -> {
            RequestWeeklyReportDialog dialog = new RequestWeeklyReportDialog(mainFrame);
            dialog.setVisible(true);
        });

        add(server);

        bugzilla.setMnemonic('B');
        bugzilla.setEnabled(false);

        JMenuItem configure = new JMenuItem("Configure...");
        configure.addActionListener(e -> {
            BugzillaConfiguration config = new BugzillaConfiguration(mainFrame);
            config.setVisible(true);
        });
        bugzilla.add(configure);

        JMenuItem refresh = new JMenuItem("Refresh");
        refresh.addActionListener(e -> {
            BugzillaRefresh packet = new BugzillaRefresh(RequestID.nextRequestID());
            mainFrame.getConnection().sendPacket(packet);
        });
        bugzilla.add(refresh);
        add(bugzilla);

        JMenu window = new JMenu("Window");
        JMenuItem restoreDefaultLayout = new JMenuItem("Restore Default Layout");
        restoreDefaultLayout.addActionListener(e -> {
            DockingState.restoreApplicationLayout(DockingLayouts.getLayout("default"));
        });

        window.add(restoreDefaultLayout);

        JMenu theme = new JMenu("Theme");
        window.add(theme);

        theme.add(new ThemeItem("Arc", "com.formdev.flatlaf.intellijthemes.FlatArcIJTheme"));
        theme.add(new ThemeItem("Solarized Dark", "com.formdev.flatlaf.intellijthemes.FlatSolarizedDarkIJTheme"));
        theme.add(new ThemeItem("Solarized Light", "com.formdev.flatlaf.intellijthemes.FlatSolarizedLightIJTheme"));

        add(window);

        if (System.getenv("TASK_GLACIER_DEV_INSTANCE") != null) {
            JMenuItem random = new JMenuItem("Random Tasks");
            window.add(random);

            random.addActionListener(e -> {
                List<String> list = Arrays.asList("Wipe down countertops",
                        "Clean the sink",
                        "Empty the dishwasher",
                        "Load the dishwasher",
                        "Wipe down the stovetop",
                        "Clean the microwave inside and out",
                        "Wipe down appliance exteriors (refrigerator, oven, dishwasher)",
                        "Clean out the refrigerator",
                        "Wipe down cabinet fronts",
                        "Sweep the kitchen floor",
                        "Mop the kitchen floor",
                        "Take out the trash",
                        "Organize a pantry shelf",
                        "Clean the coffee maker",
                        "Descale the kettle",
                        "Clean the toaster/toaster oven",
                        "Wipe down the kitchen table",
                        "Clean under the sink",
                        "Wash dish towels",
                        "Sanitize sponges",
                        "Clean the garbage disposal",
                        "Wipe down light switches",
                        "Clean the range hood filters",
                        "Deep clean the oven",
                        "Wipe down the backsplash",
                        "Organize a junk drawer",
                        "Check expiration dates on food",
                        "Clean the floor mats",
                        "Wipe down the high chairs",
                        "Clean crumbs from drawers",
                        "Polish stainless steel appliances",
                        "Clean the spice rack",
                        "Wipe down the dining chairs",
                        "Clean out the vegetable crisper",
                        "Defrost the freezer (if not frost-free)",
                        "Clean kitchen windows/blinds",
                        "Scrub the grout lines",
                        "Wash the fruit bowl",
                        "Clean the dish drying rack",
                        "Wipe down the cutting boards",
                        "Organize food storage containers",
                        "Clean the ice maker",
                        "Wipe down the can opener",
                        "Clean out the bread box",
                        "Wash the oven mitts",
                        "Clean the kitchen baseboards",
                        "Sanitize the recycling bin",
                        "Clean the dog/cat food bowls",
                        "Wipe down the bar stools",
                        "Clean the water dispenser on the fridge",
                        "Clean the toilet (bowl, seat, exterior)",
                        "Wipe down the vanity and sink",
                        "Clean the mirror",
                        "Scrub the shower/tub",
                        "Wipe down bathroom counters",
                        "Sweep the bathroom floor",
                        "Mop the bathroom floor",
                        "Empty the trash can",
                        "Change out bath towels",
                        "Clean the showerhead",
                        "Wipe down cabinet fronts",
                        "Clean under the sink",
                        "Organize medicine cabinet",
                        "Wipe down light fixtures",
                        "Clean the soap dish",
                        "Sanitize toothbrush holder",
                        "Clean shower curtain/liner",
                        "Wash bath mats",
                        "Wipe down baseboards",
                        "Clean bathroom fan cover",
                        "Scrub the grout in the shower/floor",
                        "Clean the doorknobs",
                        "Wipe down walls near the toilet",
                        "Clean the drain stopper",
                        "Refill soap dispensers",
                        "Organize toiletries",
                        "Clean the exhaust fan cover",
                        "Wipe down the toilet paper holder",
                        "Clean the towel racks",
                        "Dust light fixtures",
                        "Clean windows/blinds",
                        "Wipe down the outside of trash can",
                        "Clean the scale",
                        "Organize hair products",
                        "Clean the bidet (if applicable)",
                        "Wipe down the plunger and toilet brush holder",
                        "Clean any decorative items",
                        "Sanitize the toilet handle",
                        "Wipe down the shower caddy",
                        "Clean the bath toys",
                        "Organize cleaning supplies under the sink",
                        "Clean the back of the toilet",
                        "Dust shelves",
                        "Wipe down the shower door track",
                        "Clean the wall tiles",
                        "Wash hand towels",
                        "Clean the tub faucet",
                        "Wipe down the hamper",
                        "Clean the shower base",
                        "Dust all surfaces (tables, shelves, TV stand)",
                        "Vacuum the carpet/rugs",
                        "Sweep/mop hard floors",
                        "Fluff pillows on sofa/chairs",
                        "Straighten blankets",
                        "Wipe down coffee table/end tables",
                        "Clean TV screen (with appropriate cleaner)",
                        "Dust blinds/curtains",
                        "Clean windows",
                        "Dust ceiling fan",
                        "Wipe down light switches",
                        "Organize magazines/books",
                        "Put away remote controls",
                        "Clean baseboards",
                        "Dust picture frames",
                        "Water plants",
                        "Wipe down door handles",
                        "Clean light fixtures",
                        "Vacuum under sofa cushions",
                        "Clean up pet toys/beds",
                        "Dust lampshades",
                        "Organize media (DVDs, games)",
                        "Clean the fireplace mantel",
                        "Wipe down the entertainment center",
                        "Dust air vents",
                        "Clean and organize decorative items",
                        "Wipe down the back of the TV",
                        "Vacuum drapes",
                        "Clean out the fireplace (if used)",
                        "Dust any sculptures or figurines",
                        "Clean the remote controls",
                        "Wipe down the front door (interior)",
                        "Organize shoes near the entryway",
                        "Clean the entry rug",
                        "Dust speakers",
                        "Wipe down window sills",
                        "Clean mirrors",
                        "Dust any trophies or awards",
                        "Organize board games",
                        "Clean the thermostat",
                        "Wipe down the console table",
                        "Dust artificial plants",
                        "Clean the walls of fingerprints",
                        "Organize throw pillows",
                        "Dust the top of tall furniture",
                        "Clean the sliding glass door",
                        "Wipe down the soundbar",
                        "Dust the router/modem",
                        "Clean any glass tabletops",
                        "Organize coats in entryway closet",
                        "Make the bed",
                        "Put away clothes",
                        "Dust nightstands",
                        "Dust dresser",
                        "Vacuum/sweep/mop floor",
                        "Clean mirrors",
                        "Dust ceiling fan",
                        "Wipe down light switches",
                        "Clean windows/blinds",
                        "Empty trash can",
                        "Dust baseboards",
                        "Organize clothes in closet",
                        "Change bed linens",
                        "Dust picture frames",
                        "Wipe down door handles",
                        "Dust lamps",
                        "Organize books on shelves",
                        "Clean under the bed",
                        "Wipe down headboard",
                        "Dust fan blades",
                        "Clean out dresser drawers",
                        "Put away laundry",
                        "Wipe down alarm clock",
                        "Clean out nightstand drawers",
                        "Organize jewelry",
                        "Dust top of wardrobe",
                        "Clean any desk surface",
                        "Wipe down the closet door",
                        "Dust air vents",
                        "Organize shoes",
                        "Clean makeup brushes",
                        "Wipe down the back of the bedroom door",
                        "Dust any decorative items",
                        "Clean bedroom windowsills",
                        "Organize paperwork on desk",
                        "Dust blinds",
                        "Wipe down the bed frame",
                        "Clean the rug",
                        "Dust window treatments",
                        "Clean the walls",
                        "Organize the top of dresser",
                        "Wipe down the mirror frame",
                        "Clean out the hamper",
                        "Dust any wall art",
                        "Organize charging cables",
                        "Wipe down the doorknob",
                        "Clean the floor under the dresser",
                        "Dust and wipe down any chairs",
                        "Organize any exercise equipment",
                        "Wipe down washer and dryer exteriors",
                        "Clean out the lint trap",
                        "Sweep the floor",
                        "Mop the floor",
                        "Wipe down shelves",
                        "Clean laundry sink",
                        "Organize laundry supplies",
                        "Wipe down countertops",
                        "Empty trash can",
                        "Clean the dryer vent (deeper clean)",
                        "Wipe down light switches",
                        "Dust baseboards",
                        "Clean the walls from splashes",
                        "Wipe down the ironing board",
                        "Organize hangers",
                        "Clean the floor drain",
                        "Wipe down the door",
                        "Dust overhead lighting",
                        "Clean the area behind washer/dryer",
                        "Wipe down any folding tables",
                        "Check for leaks",
                        "Clean the detergent dispenser",
                        "Wipe down the window",
                        "Dust any pipes/hoses",
                        "Organize cleaning cloths",
                        "Clean out the utility sink trap",
                        "Wipe down the wall where the machines are",
                        "Clean any pet hair from the floor",
                        "Organize spare light bulbs",
                        "Wipe down any storage containers",
                        "Clean the dryer drum",
                        "Wipe down the washer drum (front loader gasket)",
                        "Dust around the water hookups",
                        "Organize the lost and found basket",
                        "Clean any lint build-up in the area",
                        "Wipe down the top of the machines",
                        "Clean the floor mats",
                        "Dust shelves above machines",
                        "Wipe down the door frame",
                        "Clean the area where the laundry basket sits",
                        "Organize the laundry sorter",
                        "Wipe down the base of the machines",
                        "Clean the utility sink faucet",
                        "Dust the water heater (if in laundry room)",
                        "Wipe down any wall art",
                        "Clean the dryer exhaust hose (if accessible)",
                        "Organize the various laundry additives",
                        "Wipe down the back wall of the laundry room",
                        "Clean the ceiling",
                        "Dust desk surface",
                        "Wipe down computer screen",
                        "Clean keyboard and mouse",
                        "Organize papers",
                        "Empty trash can",
                        "Dust shelves and bookcases",
                        "Vacuum/sweep/mop floor",
                        "Dust blinds/windows",
                        "Wipe down light switches",
                        "Organize pens/supplies",
                        "Clean baseboards",
                        "Dust picture frames",
                        "Water plants",
                        "Wipe down chair",
                        "Clean office chair wheels",
                        "Organize cables",
                        "Dust printer/scanner",
                        "Clean phone",
                        "Wipe down file cabinet",
                        "Dust lamps",
                        "Organize files",
                        "Clean office windows",
                        "Wipe down the door",
                        "Dust any decorative items",
                        "Clean the calendar",
                        "Wipe down the outside of storage boxes",
                        "Dust fan blades",
                        "Organize stationery",
                        "Clean a whiteboard/corkboard",
                        "Wipe down the wall around your desk",
                        "Dust the modem/router",
                        "Organize charging cords",
                        "Clean the wastebasket",
                        "Wipe down the front of drawers",
                        "Dust the speakers",
                        "Organize reference books",
                        "Clean any monitors",
                        "Wipe down the chair armrests",
                        "Dust any personal photos",
                        "Organize business cards",
                        "Clean the floor under the desk",
                        "Wipe down any wall-mounted items",
                        "Dust the top of shelves",
                        "Organize the desk drawers",
                        "Clean the trackpad (if laptop)",
                        "Wipe down the power strip",
                        "Dust any awards or certificates",
                        "Organize receipts",
                        "Clean the desk lamp",
                        "Vacuum all carpets/rugs",
                        "Sweep all hard floors",
                        "Mop all hard floors",
                        "Empty all trash cans",
                        "Dust all surfaces",
                        "Clean all mirrors",
                        "Wipe down all door handles",
                        "Clean all light switches",
                        "Dust all ceiling fans",
                        "Clean all windows (interior)",
                        "Water all houseplants",
                        "Take out recycling",
                        "Clean all baseboards",
                        "Dust all blinds/curtains",
                        "Vacuum upholstered furniture",
                        "Spot clean walls",
                        "Clean pet stains",
                        "Dust all air vents",
                        "Clean exterior of all doors (interior side)",
                        "Organize linen closet",
                        "Change air filters",
                        "Test smoke detectors",
                        "Replace light bulbs",
                        "Check batteries in remotes",
                        "Clean all exterior windows",
                        "Wash throw blankets",
                        "Clean entryway",
                        "Organize utility closet",
                        "Wipe down all stair railings",
                        "Dust light fixtures",
                        "Clean under all rugs",
                        "Vacuum pet beds",
                        "Organize cleaning supplies",
                        "Clean inside of all closets",
                        "Wipe down all door frames",
                        "Dust top of all doors",
                        "Clean all glass tables",
                        "Wipe down all window sills",
                        "Clean and organize the mudroom",
                        "Dust all picture frames",
                        "Organize mail",
                        "Clean all floor registers",
                        "Wipe down the washing machine exterior",
                        "Clean out the dryer lint trap",
                        "Dust all wall art",
                        "Organize all shoes near entryways",
                        "Clean all doormats",
                        "Wipe down any banisters",
                        "Dust all fire alarms",
                        "Clean all sliding glass door tracks",
                        "Wipe down all appliance fronts",
                        "Clean any small appliances",
                        "Sweep front porch/steps",
                        "Wipe down outdoor furniture",
                        "Water outdoor plants",
                        "Sweep garage floor",
                        "Organize garage tools",
                        "Clean outdoor lighting fixtures",
                        "Wipe down front door (exterior)",
                        "Clean outdoor trash cans",
                        "Sweep back patio/deck",
                        "Clean grill exterior",
                        "Wipe down outdoor windows",
                        "Clean gutters (if accessible/safe)",
                        "Tidy up garden beds",
                        "Pick up mail/flyers",
                        "Hose down driveway/walkway",
                        "Clean pet waste from yard",
                        "Organize recycling bins",
                        "Wipe down exterior of shed",
                        "Sweep garage entryway",
                        "Clean outdoor rugs",
                        "Wipe down railings",
                        "Dust outdoor light sensors",
                        "Clean bird feeders",
                        "Organize gardening tools",
                        "Wipe down outdoor doorknobs",
                        "Clean outdoor welcome mat",
                        "Sweep around the foundation",
                        "Clean window screens",
                        "Wipe down outdoor speakers",
                        "Organize children's outdoor toys",
                        "Clean the mailbox",
                        "Wipe down the garden hose",
                        "Sweep the carport",
                        "Clean out the car (if parked in driveway)",
                        "Wipe down the grill cover",
                        "Organize firewood",
                        "Clean the outdoor spigots",
                        "Wipe down the house numbers",
                        "Clean the doorbell",
                        "Sweep the side paths",
                        "Wipe down any outdoor art",
                        "Organize potted plants",
                        "Clean the garage door exterior",
                        "Wipe down the fence gate",
                        "Clean the exterior of the shed windows",
                        "Dust the outside light fixtures",
                        "Wipe down the house siding (spot clean)",
                        "Clean the patio furniture cushions",
                        "Organize garden hoses",
                        "Wipe down the garage door opener");

                int i = 0;
                for (String str : list) {
                    i++;
                    if (i > 20) {
                        break;
                    }
                    CreateTask create = new CreateTask(str, 0, RequestID.nextRequestID());
                    mainFrame.getConnection().sendPacket(create);
                }
            });
        }
    }

    class ThemeItem extends JCheckBoxMenuItem {
        public ThemeItem(String name, String className) {
            addActionListener(e -> {
                try {
                    UIManager.setLookAndFeel(className);
                } catch (ClassNotFoundException | InstantiationException | IllegalAccessException |
                         UnsupportedLookAndFeelException ex) {
                    throw new RuntimeException(ex);
                }
            });
        }
    }
    public void connected() {
        add.setEnabled(true);
        connect.setEnabled(false);
        disconnect.setEnabled(true);
        requestDailyReport.setEnabled(true);
        bugzilla.setEnabled(true);
    }

    public void disconnected() {
        add.setEnabled(false);
        connect.setEnabled(true);
        disconnect.setEnabled(false);
        requestDailyReport.setEnabled(false);
        bugzilla.setEnabled(false);
    }
}
