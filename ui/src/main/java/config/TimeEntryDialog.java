package config;

import data.TimeData;
import util.DialogEscape;
import util.LabeledComponent;

import javax.swing.*;
import java.awt.*;
import java.util.Optional;

class TimeEntryDialog extends JDialog {
    TimeData.TimeCategory category = null;
    TimeData.TimeCode code = null;

    TimeEntryDialog(Window parent, TimeData timeData) {
        super(parent);

        setTitle("Time Entry Selection");

        setModalityType(ModalityType.APPLICATION_MODAL);

        DialogEscape.addEscapeHandler(this);

        setLayout(new GridBagLayout());

        JButton add = new JButton("Add");
        add.setEnabled(false);

        JComboBox<String> timeCategory = new JComboBox<>();
        JComboBox<String> timeCode = new JComboBox<>();

        timeCategory.addActionListener(e -> {
            Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                    .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                    .findFirst();

            timeCode.removeAllItems();
            add.setEnabled(false);

            if (timeCategory2.isPresent()) {
                for (TimeData.TimeCode code : timeCategory2.get().timeCodes) {
                    timeCode.addItem(code.name);
                }
                timeCode.setSelectedItem(null);
            }

            SwingUtilities.invokeLater(() -> {
                pack();
                setLocationRelativeTo(parent);
            });
        });

        timeCode.addActionListener(e -> add.setEnabled(true));
        for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
            timeCategory.addItem(category.name);
        }

        timeCategory.setSelectedItem(null);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(15, 15, 5, 15);
        gbc.gridx = 0;
        gbc.gridy = 0;

        add(new LabeledComponent("Time Category", timeCategory), gbc);
        gbc.gridy++;
        gbc.insets = new Insets(5, 15, 5, 15);
        add(new LabeledComponent("Time Code", timeCode), gbc);
        gbc.gridy++;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets(5, 15, 15, 15);
        add(add, gbc);

        add.addActionListener(e -> {
            Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                    .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                    .findFirst();

//            TimeEntry.Row row = new TimeEntry.Row();
            category = timeCategory2.get();
            code = timeCategory2.get().timeCodes.stream()
                    .filter(timeCode1 -> timeCode1.name.equals(timeCode.getSelectedItem()))
                    .findFirst().get();
//
//            // if we already have a code from this category, remove it
//            for (int i = 0; i < timeEntry.model.data.size(); i++) {
//                if (timeEntry.model.data.get(i).category == row.category) {
//                    timeEntry.model.data.remove(i);
//                    timeEntry.model.fireTableRowsDeleted(i, i);
//                    break;
//                }
//            }
//            timeEntry.model.data.add(row);
//            timeEntry.model.fireTableRowsInserted(timeEntry.model.data.size() - 1, timeEntry.model.data.size() - 1);

            dispose();
        });

        pack();

        setLocationRelativeTo(parent);
    }

    public static TimeData.TimeEntry display(Window parent, TimeData timeData) {
        TimeEntryDialog dialog = new TimeEntryDialog(parent, timeData);
        dialog.setVisible(true);
        System.out.println("Done");
        if (dialog.category == null || dialog.code == null) {
            return null;
        }
        return new TimeData.TimeEntry(dialog.category, dialog.code);
    }
}
