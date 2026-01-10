package tray;

import data.TimeData;
import packets.DailyReportMessage;
import taskglacier.MainFrame;
import tree.ElapsedTimeCellRenderer;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

public class SystemTrayDailyReport extends JPanel {
    private static class Value {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        long minutes;

        public int getCategoryID() {
            return category.id;
        }

        public long getMinutes() {
            return minutes;
        }
    }

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

        List<Value> values = new ArrayList<>();

        report.timesPerTimeEntry.forEach((timeEntry, instant) -> {
            Value value = new Value();

            value.category = timeEntry.category;
            value.code = timeEntry.code;
            value.minutes = TimeUnit.MILLISECONDS.toMinutes(instant.toEpochMilli());

            values.add(value);
        });

        Collections.sort(values, Comparator.comparing(Value::getCategoryID)
                .reversed()
                .thenComparing(Value::getMinutes)
                .reversed()
        );

        for (Value value : values) {
            model.addRow(new Object[] { value.category.name + " - " + value.code.name, value.minutes });
        }
        model.fireTableDataChanged();
    }
}
