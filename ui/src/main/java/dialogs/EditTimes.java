package dialogs;

import data.Task;
import data.TimeData;
import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/*
Display a selection for the time category and code to use for this task

In addition to that, provide advanced options for modifying the time category and code for past start/stops
 */
public class EditTimes extends JDialog {
    private final TimeData timeData;

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
    }

    class TableModel extends AbstractTableModel {
        List<Row> data = new ArrayList<>();

        @Override
        public int getRowCount() {
            return data.size();
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            Row row = data.get(rowIndex);
            if (columnIndex == 0) {
                return row.category.name;
            }
            return row.code;
        }
    }

    public EditTimes(MainFrame mainFrame, Task task) {
        timeData = mainFrame.getTimeData();

        JButton add = new JButton("+");
        JButton remove = new JButton("-");
        add(add);
        add(remove);


        JComboBox<String> timeCategory = new JComboBox<>();
        JComboBox<String> timeCode = new JComboBox<>();

        for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
            timeCategory.addItem(category.name);
        }

        setLayout(new FlowLayout());

        add(new JLabel("Time Category"));
        add(timeCategory);
        add(new JLabel("Time Code"));
        add(timeCode);

        timeCategory.addActionListener(e -> {
            Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                    .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                    .findFirst();

            if (timeCategory2.isPresent()) {
                timeCode.removeAllItems();
                for (TimeData.TimeCode code : timeCategory2.get().timeCodes) {
                    timeCode.addItem(code.name);
                }
            }
        });

        timeCode.addActionListener(e -> {

        });


        JButton save = new JButton("Save");

        save.addActionListener(e -> {
            UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task);
            update.timeCodes.clear();

            mainFrame.getConnection().sendPacket(update);

            EditTimes.this.dispose();
        });


        add(save);

        TableModel model = new TableModel();
        JTable table = new JTable(model);

        add(new JScrollPane(table));

        add.addActionListener(e -> {
            Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                    .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                    .findFirst();

            Row row = new Row();
            row.category = timeCategory2.get();
            row.code = timeCategory2.get().timeCodes.stream()
                    .filter(timeCode1 -> timeCode1.name.equals(timeCode.getSelectedItem()))
                    .findFirst().get();

            model.data.add(row);
            model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);
        });

        setSize(200, 200);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
