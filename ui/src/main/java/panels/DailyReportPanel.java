package panels;

import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;
import packets.RequestDailyReport;
import packets.RequestID;
import taskglacier.MainFrame;
import tree.DailyReportTreeTable;

import javax.swing.*;
import java.awt.*;
import java.time.LocalDate;

public class DailyReportPanel extends JPanel implements Dockable {
    JLabel date = new JLabel();
    DailyReportTreeTable newTable = new DailyReportTreeTable();
    private MainFrame mainFrame;

    private boolean today = false;

    @DockingProperty(name = "month", required = true)
    private int month;
    @DockingProperty(name = "day", required = true)
    private int day;
    @DockingProperty(name = "year", required = true)
    private int year;

    private DailyReportMessage.DailyReport report = null;

    private String persistentID;
    private String titleText;
    private String tabText;

    // represents the current day always
    public DailyReportPanel(MainFrame mainFrame) {
        this.mainFrame = mainFrame;

        today = true;

        LocalDate now = LocalDate.now();

        month = now.getMonthValue();
        day = now.getDayOfMonth();
        year = now.getYear();

        persistentID = "daily-report-today";
        titleText = "Daily Report (Today)";
        tabText = "Daily Report (Today)";

        Docking.registerDockable(this);

        buildUI();
    }

    public DailyReportPanel(MainFrame mainFrame, LocalDate date) {
        this.mainFrame = mainFrame;
        month = date.getMonthValue();
        day = date.getDayOfMonth();
        year = date.getYear();

        persistentID = String.format("daily-report-%d-%d-%d", month, day, year);
        titleText = String.format("Daily Report (%d/%d/%d)", month, day, year);
        tabText = String.format("Daily Report (%d/%d/%d)", month, day, year);

        Docking.registerDockable(this);

        buildUI();
    }

    public DailyReportPanel(DynamicDockableParameters parameters) {
        persistentID = parameters.getPersistentID();
        titleText = parameters.getTitleText();
        tabText = parameters.getTabText();

        Docking.registerDockable(this);

        buildUI();
    }

    private void refreshTodayDate() {
        if (!today) {
            return;
        }
        LocalDate now = LocalDate.now();

        month = now.getMonthValue();
        day = now.getDayOfMonth();
        year = now.getYear();
    }
    public int getMonth() {
        refreshTodayDate();
        return month;
    }

    public int getDay() {
        refreshTodayDate();
        return day;
    }

    public int getYear() {
        refreshTodayDate();
        return year;
    }

    @Override
    public void updateProperties() {
        mainFrame = MainFrame.mainFrame;

        refreshTodayDate();

        if (mainFrame.isConnected()) {
            RequestDailyReport request = new RequestDailyReport();
            request.requestID = RequestID.nextRequestID();
            request.month = month;
            request.day = day;
            request.year = year;

            mainFrame.getConnection().sendPacketWhenReady(request);
        }
    }

    private void buildUI() {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        JLabel start = new JLabel();
        JLabel end = new JLabel();
        JLabel total = new JLabel();

        add(start, gbc);
        gbc.gridy++;
        add(end, gbc);
        gbc.gridy++;
        add(total, gbc);
        gbc.gridy++;

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(newTable), gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();

        newTable.update(report);

        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTitleText() {
        return titleText;
    }

    @Override
    public String getTabText() {
        return tabText;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }
}
