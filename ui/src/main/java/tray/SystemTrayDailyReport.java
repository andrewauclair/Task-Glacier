package tray;

import packets.DailyReportMessage;
import packets.RequestDailyReport;
import packets.RequestID;
import raven.datetime.DatePicker;
import taskglacier.MainFrame;
import tree.ElapsedTimeCellRenderer;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.time.Instant;
import java.time.LocalDate;
import java.util.concurrent.TimeUnit;

public class SystemTrayDailyReport extends JPanel {
    private DefaultTableModel model = new DefaultTableModel(0, 2) {
        @Override
        public Class<?> getColumnClass(int columnIndex) {
            if (columnIndex == 0) {
                return String.class;
            }
            return long.class;
        }
    };
    private JTable table = new JTable(model);

    private DailyReportMessage.DailyReport report = null;

    public SystemTrayDailyReport(MainFrame mainFrame) {
        super(new BorderLayout());

        add(new JScrollPane(table));

        table.getColumnModel().getColumn(1).setCellRenderer(new ElapsedTimeCellRenderer());
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();

        model.setRowCount(0);

        report.timesPerTimeEntry.forEach((timeEntry, instant) -> {
            long minutes = TimeUnit.MILLISECONDS.toMinutes(instant.toEpochMilli());

            model.addRow(new Object[] { timeEntry.category.name + " - " + timeEntry.code.name, minutes });
        });

        model.fireTableDataChanged();
    }
}
